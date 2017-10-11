/* Copyright (C) 2003,  All Rights Reserved. */

/* This module implements the LACPDU formating/parsing routines. */

#include "lacp_types.h"
#include "lacpdu.h"
#include "lacpd.h"
#include "lacp_functions.h"
#include "lacp_main.h"

const unsigned char lacp_grp_addr[LACP_GRP_ADDR_LEN] = 
  { 0x01, 0x80, 0xC2, 0x00, 0x00, 0x02 };

void
lacp_dump_packet (const unsigned char * const pdu)
{
  zlog_debug (LACPM, "PDU-00: 0x%.02x - lacp subtype", pdu[0]);
  zlog_debug (LACPM, "PDU-01: 0x%.02x - lacp version", pdu[1]);
  zlog_debug (LACPM, "PDU-02: 0x%.02x - actor information tlv", pdu[2]);
  zlog_debug (LACPM, "PDU-03: 0x%.02x - actor information length", pdu[3]);
  zlog_debug (LACPM, "PDU-04: 0x%.02x%.02x - system priority: %d", 
      pdu[4], pdu[5], pdu[4] << 8 | pdu[5]);
  zlog_debug (LACPM, "PDU-06: %.02x%.02x%.02x%.02x%.02x%.02x - system id", 
      pdu[6], pdu[7], pdu[8], pdu[9], pdu[10], pdu[11]);

  zlog_debug (LACPM, "PDU-12: 0x%.02x%.02x - oper port key: %d", 
      pdu[12], pdu[13], pdu[12] << 8 | pdu[13]);

  zlog_debug (LACPM, "PDU-14: 0x%.02x%.02x - oper port priority: %d", 
      pdu[14], pdu[15], pdu[14] << 8 | pdu[15]);

  zlog_debug (LACPM, "PDU-16: 0x%.02x%.02x - oper port number: %d", 
      pdu[16], pdu[17], pdu[16] << 8 | pdu[17]);

  zlog_debug (LACPM, "PDU-18: 0x%.02x - port state", pdu[18]);

  zlog_debug (LACPM, "PDU-19: 0x%.02x", pdu[19]);
  zlog_debug (LACPM, "PDU-20: 0x%.02x", pdu[20]);
  zlog_debug (LACPM, "PDU-21: 0x%.02x", pdu[21]);

  zlog_debug (LACPM, "PDU-22: 0x%.02x - partner information tlv", pdu[22]);
  zlog_debug (LACPM, "PDU-23: 0x%.02x - partner information length", pdu[23]);

  zlog_debug (LACPM, "PDU-24: 0x%.02x%.02x - partner oper system priority: %d", 
      pdu[24], pdu[25], pdu[24] << 8 | pdu[25]);

  zlog_debug (LACPM, "PDU-26: %.02x%.02x%.02x%.02x%.02x%.02x - partner oper system", 
      pdu[26], pdu[27], pdu[28], pdu[29], pdu[30], pdu[31]);

  zlog_debug (LACPM, "PDU-32: 0x%.02x%.02x - partner oper key: %d", 
      pdu[32], pdu[33], pdu[32] << 8 | pdu[33]);

  zlog_debug (LACPM, "PDU-34: 0x%.02x%.02x - partner oper port priority: %d", 
      pdu[34], pdu[35], pdu[34] << 8 | pdu[35]);

  zlog_debug (LACPM, "PDU-36: 0x%.02x%.02x - partner oper port number: %d", 
      pdu[36], pdu[37], pdu[36] << 8 | pdu[37]);

  zlog_debug (LACPM, "PDU-38: 0x%.02x - partner port state", pdu[38]);

  zlog_debug (LACPM, "PDU-39: 0x%.02x", pdu[39]);
  zlog_debug (LACPM, "PDU-40: 0x%.02x", pdu[40]);
  zlog_debug (LACPM, "PDU-41: 0x%.02x", pdu[41]);

  zlog_debug (LACPM, "PDU-42: 0x%.02x - collector information tlv", pdu[42]);
  zlog_debug (LACPM, "PDU-43: 0x%.02x - collector information length", pdu[43]);

  zlog_debug (LACPM, "PDU-44: 0x%.02x%.02x - collector max delay: %d", 
      pdu[44], pdu[45], pdu[44] << 8 | pdu[45]);
}

/* This module implements the LACPDU transmit and receive functions. 
  43.4.2.2 */

int
format_lacpdu (struct lacp_link * link, register unsigned char * bufptr)
{

  if ((bufptr == NULL) || (link == NULL))
    return RESULT_ERROR;

  /* Subtype */
  bufptr[0] = LACP_SUBTYPE;
  bufptr[1] = LACP_VERSION;
  bufptr[2] = LACP_ACTOR_INFORMATION_TLV;
  bufptr[3] = LACP_ACTOR_INFORMATION_LENGTH;

  bufptr[4] = LACP_SYSTEM_PRIORITY >> 8;
  bufptr[5] = LACP_SYSTEM_PRIORITY & 0xff;

  pal_mem_cpy (&bufptr[6], LACP_SYSTEM_ID, 
               LACP_GRP_ADDR_LEN);

  bufptr[12] = link->actor_oper_port_key >> 8;
  bufptr[13] = link->actor_oper_port_key & 0xff;

  bufptr[14] = link->actor_port_priority >> 8;
  bufptr[15] = link->actor_port_priority & 0xff;

  bufptr[16] = link->actor_port_number >> 8;
  bufptr[17] = link->actor_port_number & 0xff;

  bufptr[18] = link->actor_oper_port_state;

  bufptr[19] = bufptr[20] = bufptr[21] = '\0';

  bufptr[22] = LACP_PARTNER_INFORMATION_TLV;
  bufptr[23] = LACP_PARTNER_INFORMATION_LENGTH;

  bufptr[24] = link->partner_oper_system_priority >> 8;
  bufptr[25] = link->partner_oper_system_priority & 0xff;

  pal_mem_cpy (&bufptr[26], link->partner_oper_system, LACP_GRP_ADDR_LEN);

  bufptr[32] = link->partner_oper_key >> 8;
  bufptr[33] = link->partner_oper_key & 0xff;

  bufptr[34] = link->partner_oper_port_priority >> 8;
  bufptr[35] = link->partner_oper_port_priority & 0xff;

  bufptr[36] = link->partner_oper_port_number >> 8;
  bufptr[37] = link->partner_oper_port_number & 0xff;

  bufptr[38] = link->partner_oper_port_state;

  bufptr[39] = bufptr[40] = bufptr[41] = '\0';

  bufptr[42] = LACP_COLLECTOR_INFORMATION_TLV;
  bufptr[43] = LACP_COLLECTOR_INFORMATION_LENGTH;

  bufptr[44] = LACP_COLLECTOR_MAX_DELAY >> 8;
  bufptr[45] = LACP_COLLECTOR_MAX_DELAY & 0xff;

  /* Faster than memcpy - uglier too! */
  bufptr[46] = bufptr[47] = bufptr[48] = bufptr[49] =
  bufptr[50] = bufptr[51] = bufptr[52] = bufptr[53] =
  bufptr[54] = bufptr[55] = bufptr[56] = bufptr[57] = '\0';

  bufptr[58] = LACP_TERMINATOR_TLV;
  bufptr[59] = LACP_TERMINATOR_LENGTH;

  pal_mem_set (&bufptr[60], 0, 50);

  return RESULT_OK;
}

/* Parse a pdu into a more palatable format. Returns RESULT_ERROR 
  if PDU is invalid, RESULT_OK if not. */

int
parse_lacpdu (const unsigned char * const buf, const int len,
                struct lacp_pdu * const lacp_info)
{
  u_int8_t partner_info_len, actor_info_len;
  u_int8_t collector_info_len, terminator_info_len;

  if (buf == NULL || lacp_info == NULL)
    {
      return RESULT_ERROR;
    }

  if (buf[0] != LACP_SUBTYPE)
    return RESULT_ERROR;

  /* Add length check here. */

  lacp_info->subtype = buf[0];
  lacp_info->version_number = buf[1];

  actor_info_len = buf [LACP_ACTOR_INFO_LEN_OFFSET];

  if (actor_info_len != LACP_ACTOR_INFO_LEN_VALID)
    return RESULT_ERROR;

  /* Actor information */
  lacp_info->actor_system_priority = (buf[4] << 8) | buf[5];
  pal_mem_cpy (lacp_info->actor_system, &buf[6], LACP_GRP_ADDR_LEN);
  lacp_info->actor_key = (buf[12]  << 8) | buf[13];
  lacp_info->actor_port_priority  = (buf[14] << 8) | buf[15];
  lacp_info->actor_port_number = (buf[16] << 8) | buf[17];

  if (lacp_info->actor_port_number < LACP_PORT_NUM_MIN)
    return RESULT_ERROR;

  lacp_info->actor_state = buf[18];

  partner_info_len = buf [LACP_PARTNER_INFO_LEN_OFFSET];

  if (partner_info_len != LACP_PARTNER_INFO_LEN_VALID)
    return RESULT_ERROR;

  /* Partner information */
  lacp_info->partner_system_priority = (buf[24] << 8) | buf[25];
  pal_mem_cpy (lacp_info->partner_system, &buf[26], LACP_GRP_ADDR_LEN);
  lacp_info->partner_key = (buf[32] << 8) | buf[33];
  lacp_info->partner_port_priority = (buf[34] << 8) | buf[35];
  lacp_info->partner_port = (buf[36] << 8) | buf[37];
  lacp_info->partner_state = buf[38];

  collector_info_len = buf [LACP_COLL_INFO_LEN_OFFSET];

  if (collector_info_len != LACP_COLL_INFO_LEN_VALID)
    return RESULT_ERROR;

  /* Collector information */
  lacp_info->collector_max_delay = (buf[44] << 8) | buf[45];

  terminator_info_len = buf [LACP_TERM_INFO_LEN_OFFSET];

  if (terminator_info_len != LACP_TERM_INFO_LEN_VALID)
    return RESULT_ERROR;

  return RESULT_OK;
}


int
format_marker (struct lacp_link * link, register unsigned char * bufptr, const int response)
{

  if ((bufptr == NULL) || (link == NULL))
    return RESULT_ERROR;

  /* Subtype */
  bufptr[0] = MARKER_SUBTYPE;
  bufptr[1] = MARKER_VERSION;

  if (response)
    {
      bufptr[2] = MARKER_RESPONSE_TLV;
      bufptr[3] = MARKER_RESPONSE_LENGTH;
      bufptr[4] = link->received_marker_info.port >> 8;
      bufptr[5] = link->received_marker_info.port & 0xff;
      pal_mem_cpy (&bufptr[6], link->received_marker_info.system, LACP_GRP_ADDR_LEN);
      bufptr[12] = (link->received_marker_info.transaction_id >> 24) & 0xff;
      bufptr[13] = (link->received_marker_info.transaction_id >> 16) & 0xff;
      bufptr[14] = (link->received_marker_info.transaction_id >> 8) & 0xff;
      bufptr[15] = link->received_marker_info.transaction_id & 0xff;
    }
#if defined(LACP_MARKER_GENERATOR)
  else
    {
      bufptr[2] = MARKER_INFORMATION_TLV;
      bufptr[3] = MARKER_INFORMATION_LENGTH;
      bufptr[4] = link->actor_port_number >> 8;
      bufptr[5] = link->actor_port_number & 0xff;
      pal_mem_cpy (&bufptr[6], lacp_instance->lacp_system.system_id, 
                   LACP_GRP_ADDR_LEN);
      bufptr[12] = (link->marker_transaction_id >> 24) & 0xff;
      bufptr[13] = (link->marker_transaction_id >> 16) & 0xff;
      bufptr[14] = (link->marker_transaction_id >> 8) & 0xff;
      bufptr[15] = link->marker_transaction_id & 0xff;
    }
#endif

  bufptr[16] = bufptr[17] = '\0';

  bufptr[18] = MARKER_TERMINATOR_TLV;
  bufptr[19] = MARKER_TERMINATOR_LENGTH;

  pal_mem_set (&bufptr[20], '\0', 90);

  return RESULT_OK;
}

/* Parse a pdu into a more palatable format. Returns RESULT_ERROR 
  if PDU is invalid, RESULT_OK if not. */

int
parse_marker (const unsigned char * const buf, const int len,
                struct marker_pdu * const marker_info)
{
  register unsigned char * bufptr = (unsigned char *)buf;
  if (buf == NULL || marker_info == NULL)
    {
      return RESULT_ERROR;
    }

  if (buf[0] != MARKER_SUBTYPE)
    return RESULT_ERROR;

  marker_info->response = (bufptr[2] == MARKER_RESPONSE_TLV);
  marker_info->port = bufptr[4] << 8 | bufptr[5];
  pal_mem_cpy (marker_info->system, &bufptr[6], 
                                LACP_GRP_ADDR_LEN);
  marker_info->transaction_id = (bufptr[12] << 24) |
      (bufptr[13] << 16) | (bufptr[14] << 8) | bufptr[15]; 

  return RESULT_OK;
}
