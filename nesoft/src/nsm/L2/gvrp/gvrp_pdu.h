/* Copyright 2003  All Rights Reserved. */

#ifndef _PACOS_GVRP_PDU_H
#define _PACOS_GVRP_PDU_H

#define GVRP_ATTRIBUTE_TYPE_VLAN       0x01
#define GVRP_ATTRIBUTE_VALUE_VLAN_SIZE 0x02

/* Event Attribute offset is calculated to access the event parameter in 
   buff array in the foll manner
   ATTRIB_EVENT_OFFSET = Llc Format Size (3) +
                         Protocol Id Size (2) + Attribute Type Size (1) +
                         Attribute Length Size (1) */
#define ATTRIB_EVENT_OFFSET              7

/* GVRP Attribute Values */

/* Length, event.  */
#define GVRP_LEAVE_ALL_SIZE         2

/* Length, event, value (2 octets).  */
#define GVRP_NON_LEAVE_ALL_SIZE     4

/* Max size of GARP PDUs for GVRP.  */
#define GVRP_PDU_SIZE              1500

#define GVRP_MAX_ATTR_PER_PDU      200
#define MVRP_VECTOR_SIZE   4
#define MVRP_NUM_OF_VALUES_SIZE  2
#define MVRP_MAX_ATTR_EVENT 6
#define GVRP_MAX_PDU_OFFSET 	   1490

#define MVRP_LEAVE_ALL_RCVD      \
   no_of_values & 0xE000

#define MVRP_DECODE_NON_P2P_VECTOR      \
   do                                   \
    {                                   \
      vector = (octet[0] << 24);        \
      vector |= (octet[1] << 16);       \
      vector |= (octet[2] << 8);        \
      vector |= octet[3];               \
    } while (0)

#define MVRP_DECODE_P2P_VECTOR          \
   do                                   \
    {                                   \
      vector = (octet[0]);              \
    } while (0)

extern void
gvrp_receive_pdu (unsigned int ifindex, u_int8_t *src_mac,
                  u_char *buff_ptr, u_int32_t buff_len);

extern bool_t 
gvrp_transmit_pdu (void *application, struct gid *gid);

void
mvrp_append_attribute (struct gvrp *gvrp, u_char *buff_ptr, u_int32_t *buff_len,
                       u_int32_t *offset, enum gid_event event, int *vector);

void
mvrp_encode_pdu(struct gvrp *gvrp, u_char *buff_ptr, u_int32_t *buff_len,
                u_int32_t *offset, u_int32_t *no_of_values, u_int32_t *vector);

bool_t
gvrp_check_vid(struct gvrp *gvrp, u_int16_t vid);

void
mvrp_vector_to_octets (u_int32_t *vector, u_char *octet);


#endif /* _PACOS_GVRP_PDU_H */
