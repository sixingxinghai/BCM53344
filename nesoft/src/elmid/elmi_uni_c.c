/**@file elmi_uni_c.c
   @brief  This file contains TX and RX state machines 
   and corresonding encoding and decoding functions for UNI-C.
 */
/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "l2lib.h"
#include "thread.h"
#include "timeutil.h"
#include "tlv.h"
#include "avl_tree.h"
#include "nsm_client.h"
#include "elmi_types.h"
#include "elmi_timer.h"
#include "elmid.h"
#include "elmi_sock.h"
#include "elmi_error.h"
#include "elmi_port.h"
#include "elmi_uni_c.h"
#include "l2_debug.h"
#include "elmi_debug.h"
#include "elmi_api.h"

/* Get number from multiplier and magnitude with base 10 */
static inline u_int32_t
elmi_get_number_from_exponent (u_int16_t mul, u_int8_t mag)
{
  u_int32_t result = PAL_TRUE;

  while (mag--)
    {
      result *= ELMI_BIT_BASE;
    }

  result *= mul;

  return result;

}

/****************************************************
*             UNI-C TX Module 
****************************************************/

/* Tramsmit request to UNIN */
s_int32_t
elmi_uni_c_tx (struct elmi_port *port, u_int8_t report_type)
{
  struct elmi_master *em = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct interface *ifp = NULL;
  struct elmi_evc_status *evc_node = NULL;
  struct listnode *node = NULL;
  u_int8_t seq_num = 0;
  u_char buf[ELMI_MAX_PACKET_SIZE];
  u_char **pnt;
  u_char *bufptr = NULL;
  u_int32_t *size = NULL;
  u_int32_t padding_len = 0;
  u_int32_t buf_len = 0;
  u_int32_t length = 0;
  s_int32_t ret = 0;

  bufptr = buf;
  pnt = &bufptr;

  if (bufptr == NULL)
    return RESULT_ERROR;

  if (!port)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  if (!elmi_if || !elmi_if->elmim)
    return RESULT_ERROR;

  ifp = port->elmi_if->ifp;
  if (!ifp)
    return RESULT_ERROR;

  /* Do not send the frame, if the interface is down */
  if (!if_is_up (ifp))
    return RESULT_ERROR;

  em = elmi_if->elmim;

  buf_len = ELMI_MAX_PACKET_SIZE;
  size = &buf_len;

  port->rcvd_response = PAL_FALSE;

  TLV_ENCODE_PUTC (ELMI_PROTO_VERSION);

  /* Message Type */
  TLV_ENCODE_PUTC (ELMI_MSG_TYPE_STATUS_ENQUIRY);

  TLV_ENCODE_PUTC (ELMI_REPORT_TYPE_IE_TLV);

  /* Report type length tlv */
  TLV_ENCODE_PUTC (ELMI_REPORT_TYPE_IE_LENGTH);

  /* Report type val */
  TLV_ENCODE_PUTC (report_type);

  /* Sequence Number */
  TLV_ENCODE_PUTC (ELMI_SEQUENCE_NUM_IE_TLV);

  /* Sequence Number length*/
  TLV_ENCODE_PUTC (ELMI_SEQUENCE_NUM_IE_LENGTH);

  /* check if it exceeds the maximum value(Use modulo 256) */
  if (port->sent_seq_num >= ELMI_SEQ_NUM_MAX)
    port->sent_seq_num = 0;

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_err (ZG, "[UNIC-TX]: UNIC Sent seq_num = %d Rcvd Seq = %d ",
              seq_num, port->recvd_send_seq_num);

  /* Encode Send Sequence Number val */
  TLV_ENCODE_PUTC (++port->sent_seq_num);

  /* Encode Received Sequence Number val */
  TLV_ENCODE_PUTC (port->recvd_send_seq_num);

  /* Data instance tlv type */
  TLV_ENCODE_PUTC (ELMI_DATA_INSTANCE_IE_TLV);
  TLV_ENCODE_PUTC (ELMI_DATA_INSTANCE_IE_LENGTH);

  /* Reserved byte */
  TLV_ENCODE_PUTC (ELMI_RESERVED_BYTE);

  /* Data instance val tlv */
  TLV_ENCODE_PUTL (port->data_instance);

  /* for UNI_C length will be always equal to ELMI_MIN_PACKET_SIZE */
  length = ELMI_MAX_PACKET_SIZE - (*size);
  if (length < ELMI_MIN_PACKET_SIZE)
    {
      padding_len = ELMI_MIN_PACKET_SIZE - length;
      TLV_ENCODE_PUT_EMPTY (padding_len);
      length += padding_len;
    }

  if (ELMI_DEBUG(packet, TX))
    {
      zlog_err (ZG, "[UNIC-TX]: Sending Request type %d", report_type);
      zlog_err (ZG, "[UNIC-TX]: Last Request type %d", port->last_request_type);
      zlog_err (ZG, "[UNIC-TX]: port->rcvd_response %d", port->rcvd_response);
    }

  /* Send out frame */
  ret = elmi_frame_send (elmi_if->dev_addr, elmi_dest_addr,
                         elmi_if->ifindex, buf, length);
  
  if (ret < 0)
    return ret;

  if (report_type == ELMI_REPORT_TYPE_ECHECK)
    port->stats.last_status_check_sent = pal_time_current (0);
  else if (report_type == ELMI_REPORT_TYPE_FULL_STATUS)
    {
      port->stats.last_full_status_enq_sent = pal_time_current (0);

      /* Before parsing evc status set local evc to inactive state */
      LIST_LOOP (elmi_if->evc_status_list, evc_node, node)
        {
          evc_node->active = PAL_FALSE;
          zlog_err (ZG, "[UNIC-RX]: EVC Ref id is %d", evc_node->evc_ref_id);
        }
    }

  /* Store the sent report type */
  port->last_request_type = report_type;

  /* Request sent to UNI-N */
  port->request_sent = PAL_TRUE;

  if (ELMI_DEBUG(packet, TX))
    {
      zlog_info (ZG, "UNIC outgoing frame:");

      zlog_info (ZG, "[UNI-TX] Protocol Version : %d",
                 ELMI_PROTO_VERSION);

      zlog_info (ZG, "[UNI-TX] Message : %X",
                 ELMI_MSG_TYPE_STATUS_ENQUIRY);

      zlog_info (ZG, "[UNI-TX] Report Type : %d", 
                 report_type);

      zlog_info (ZG, "[UNI-TX] Sequence Number : send(%X) Rcvd(%X)", 
                 port->sent_seq_num,
                 port->recvd_send_seq_num);

      zlog_info (ZG, "[UNI-TX] Data Instance : %d", 
                 port->data_instance);
    }

  if (ELMI_DEBUG (packet, TX))
    elmi_hexdump (buf, length);

  return ret;
}


/****************************************************
*             UNI-C RX Module 
****************************************************/
s_int32_t
elmi_unic_validate_rcvd_pdu (struct elmi_ifp *elmi_if, u_char **pnt,
                             s_int32_t *size, 
                             struct sockaddr_l2 *l2_skaddr,
                             struct elmi_pdu *pdu)
{
  struct elmi_port *port  = NULL;
  struct elmi_master *em  = NULL;
  u_int8_t report_type_tlv = 0;
  u_int8_t report_type_len = 0;

  if (!elmi_if || !elmi_if->elmi_enabled)
    return ELMI_ERR_BRIDGE_NOT_FOUND;
  
  em = elmi_if->elmim;

  if (!em)
    return RESULT_ERROR;

  port = elmi_if->port;

  /* Parse protocol version */
  TLV_DECODE_GETC (pdu->proto_ver);

  if (pdu->proto_ver != ELMI_PROTO_VERSION)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]:"
            " Received invalid protocol version");

      port->stats.inval_protocol_ver++;
      return RESULT_ERROR;
    }

  /* Parse Message Type */
  TLV_DECODE_GETC (pdu->msg_type);

  if (pdu->msg_type != ELMI_MSG_TYPE_STATUS)
    {
      /* Check if the protocol version repeated */
      if (pdu->msg_type == ELMI_PROTO_VERSION)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]:"
                      " Received Duplicate protocol version");

          /* Ignore the duplicate element */
          port->stats.duplicate_ie++;

          /* Parse Message Type */
          TLV_DECODE_GETC (pdu->msg_type);

          if (pdu->msg_type != ELMI_MSG_TYPE_STATUS)
            {
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: Meessage type not present");

              /* Increment counters */
              port->stats.mandatory_ie_missing++;
              return RESULT_ERROR;
            }
        }
      else
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: Received Invalid Message Type");

          port->stats.inval_msg_type++;

          return RESULT_ERROR;
        }
    }
  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_info (ZG, "[UNIC-RX]: Received Message Type = %d", pdu->msg_type);

  /* Parse report type TLV */
  TLV_DECODE_GETC (report_type_tlv);

  if (report_type_tlv != ELMI_REPORT_TYPE_IE_TLV)
    {
      /* Check if the message type is repeated */
      if (report_type_tlv == ELMI_MSG_TYPE_STATUS)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: Received Duplicate message type");

          /* Ignore the duplicate element */
          port->stats.duplicate_ie++;

          /* Parse report type TLV */
          TLV_DECODE_GETC (report_type_tlv);

          if (report_type_tlv != ELMI_REPORT_TYPE_IE_TLV)
            {
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: Received invalid  Report Type");

              port->stats.mandatory_ie_missing++;
              return RESULT_ERROR;
            }
        }
      else
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: Received Invalid Report Type\n");

          /* Increment counters */
          port->stats.mandatory_ie_missing++;

          return RESULT_ERROR;
        }
    }

  /* Parse report type length */
  TLV_DECODE_GETC (report_type_len);

  if (report_type_len != ELMI_REPORT_TYPE_IE_LENGTH)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Received Invalid Report length\n");

      /* Increment counters */
      port->stats.inval_mandatory_ie++;

      return RESULT_ERROR;
    }

  /* Parse report type val */
  TLV_DECODE_GETC (pdu->report_type);

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_info (ZG, "[UNIC-RX]: Received Report Type = %d", 
               pdu->report_type);

  /* Validate the report type as per section 5.6.9.2, page 28, MEF16 */
  if (port->request_sent == PAL_FALSE && 
      pdu->report_type != ELMI_REPORT_TYPE_SINGLE_EVC)
    {
      /* No Request from UNI-C, but received response from UNI-N */
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Received unsolicited status response msg");

      port->stats.unsolicited_status_rcvd++; 
      return RESULT_ERROR;
    }
  else if ((pdu->report_type == ELMI_REPORT_TYPE_ECHECK) 
      && ((port->last_request_type == ELMI_REPORT_TYPE_FULL_STATUS)
      ||(port->last_request_type == ELMI_REPORT_TYPE_FULL_STATUS_CONTD)
      ||(port->last_request_type == ELMI_INVALID_REPORT_TYPE)))
    {
      /* Requested for full status, but received ELMI Check msg */
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Invalid status response msg");

      port->stats.inval_status_resp++; 

      return RESULT_ERROR;
    } 
  else if ((pdu->report_type == ELMI_REPORT_TYPE_FULL_STATUS
           || pdu->report_type == ELMI_REPORT_TYPE_FULL_STATUS_CONTD)
           && ((port->last_request_type == ELMI_REPORT_TYPE_ECHECK)
           || (port->last_request_type == ELMI_INVALID_REPORT_TYPE)))
    {
      /* Rcvd unsolicited ELMI full status msg */
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Invalid status response msg");

      port->stats.inval_status_resp++; 

      return RESULT_ERROR;
    }
  else
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Received Valid ELMI frame from UNI-N");
    }

  return RESULT_OK;
}


/* Data instance and seq num for Full status and Full status continued */
s_int32_t
elmi_decode_seq_and_data_instance (u_char **pnt, s_int32_t *size,
                                   u_int32_t buf_len,
                                   struct elmi_port *port,
                                   u_int32_t *data_instance,
                                   u_int8_t *seq_num)
{
  u_int8_t tlv_type = 0; 
  u_int8_t tlv_length = 0; 
  u_int8_t tlv_val = 0; 
  u_int8_t send_seq_counter = 0; 
  u_int8_t rcvd_sent_seq_counter = 0; 
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if; 

  if (!elmi_if->elmim)
    return RESULT_ERROR;

  em = elmi_if->elmim;

  if (*size < ELMI_TLV_MIN_SIZE)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: No more bytes left in the frame");

      return RESULT_ERROR;
    }

  /* Parse sequence number type TLV */
  TLV_DECODE_GETC (tlv_type);

  if (tlv_type != ELMI_SEQUENCE_NUM_IE_TLV)
    {
      if (tlv_type == ELMI_REPORT_TYPE_IE_TLV)
        {
          /* Check if any IE is repeated, ignore it */
          TLV_DECODE_GETC (tlv_length);

          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: - Frame Validation -"
                      " Received duplicate TLV ignore");
            port->stats.duplicate_ie++;

          /* skip the val by length */
          TLV_DECODE_SKIP (tlv_length);
          TLV_DECODE_GETC (tlv_val);

          if (tlv_val != ELMI_SEQUENCE_NUM_IE_TLV)
            {
              port->stats.mandatory_ie_missing++;
              return RESULT_ERROR;
            }
        }
      else
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: Frame Validation -"
                      " Received Invalid sequence number TLV");

          port->stats.mandatory_ie_missing++;
          return RESULT_ERROR;
        }
    }

  /* Parse sequence number length */
  TLV_DECODE_GETC (tlv_length);
  if (!tlv_length)
    {
      port->stats.inval_mandatory_ie++;
      return RESULT_ERROR;
    }

  /* Parse Sent sequence number val */
  TLV_DECODE_GETC (send_seq_counter);
  if (send_seq_counter == 0)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Received seq num val is zero invalid");

      port->stats.inval_seq_num++;
      return RESULT_ERROR;
    }

  /* Store the sequence Number */
  *seq_num = send_seq_counter;

  /* Parse sent received sequence number val */
  TLV_DECODE_GETC (rcvd_sent_seq_counter);

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_info (ZG, "[UNIC-RX]: port->sent_seq_num = %d"
              " Rcvd seq = %d\n", port->sent_seq_num, rcvd_sent_seq_counter);
  
  if (rcvd_sent_seq_counter == ELMI_DEFAULT_VAL ||
      rcvd_sent_seq_counter != port->sent_seq_num)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]:"
            " Sequence number val mismatch");

      /* UNI_C will Ignore the frame and increment counters */ 

      port->stats.inval_seq_num++;

      return RESULT_ERROR;
    }

  /* decode data instance field */
  TLV_DECODE_GETC (tlv_val);

  if (tlv_val != ELMI_DATA_INSTANCE_IE_TLV)
    {
      if (tlv_val == ELMI_SEQUENCE_NUM_IE_TLV)
        {
          /* Check if any IE is repeated, ignore it */
          TLV_DECODE_GETC (tlv_length);

          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: Received duplicate TLV ignore");

            port->stats.duplicate_ie++;

          /* skip the val by length */
          TLV_DECODE_SKIP (tlv_length);
          TLV_DECODE_GETC (tlv_val);

          if (tlv_val != ELMI_DATA_INSTANCE_IE_TLV)
            {
              port->stats.mandatory_ie_missing++;
              return RESULT_ERROR;
            }
        }
      else
        {
          /* check if any tlv is repeated */
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_debug (ZG, "[UNIC-RX]: Received Wrong -"
                        " data instance field on port %d", elmi_if->ifindex);

          port->stats.mandatory_ie_missing++;
          return RESULT_ERROR;
        }
    }

  TLV_DECODE_GETC (tlv_length);

  if (tlv_length != ELMI_DATA_INSTANCE_IE_LENGTH)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: - Frame Validation -"
                  " Invalid data instance");

      port->stats.inval_mandatory_ie++;

      return RESULT_ERROR;
    }

  TLV_DECODE_SKIP (ELMI_UNUSED_BYTE);
  TLV_DECODE_GETL (tlv_val);

  if (tlv_val == ELMI_DEFAULT_VAL)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Invalid data instance");

      port->stats.inval_mandatory_ie++; 

      return RESULT_ERROR;
    }

  *data_instance = tlv_val;

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_debug (ZG, "[UNIC-RX]: Data Instance %d", tlv_val);

  return RESULT_OK;
}

/* Decode uni bw Info */
s_int32_t
elmi_decode_uni_bw_profile (u_char **pnt, s_int32_t *size, 
                            u_int32_t buf_len,
                            struct elmi_port *port,
                            struct elmi_uni_info *uni_info) 
{
  struct elmi_master *em = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_bw_profile *uni_bw = NULL;
  u_int8_t tlv_type = 0;
  s_int8_t bw_tlv_len = 0;
  u_int8_t tlv_val = 0;
  u_int8_t per_cos = 0;
  u_int8_t usr_pri = 0;
  u_int8_t tlv_length = 0;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  if (!uni_info)
    return RESULT_ERROR;

  if (*size < ELMI_TLV_MIN_SIZE)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: No more bytes left in the frame");

      return RESULT_ERROR;
    }

  /* Decode bandwidth profile parameters */
  TLV_DECODE_GETC (tlv_type);

  if (tlv_type != ELMI_BW_PROFILE_SUB_IE_TLV)
    {
      if (tlv_type == ELMI_UNI_ID_SUB_IE_TLV)
        {
          /* Check if any IE is repeated, ignore it */
          TLV_DECODE_GETC (tlv_length);

          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: Received duplicate TLV ignore");
          port->stats.duplicate_ie++;

          /* skip the val by length */
          TLV_DECODE_SKIP (tlv_length);
          TLV_DECODE_GETC (tlv_type);

          if (tlv_type != ELMI_BW_PROFILE_SUB_IE_TLV)
            {
              port->stats.mandatory_ie_missing++;
              return RESULT_ERROR;
            }
        }
      else
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]:"
                " UNI BW profile not present");

          port->stats.mandatory_ie_missing++;
          return RESULT_ERROR;
        }
    }
  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_err (ZG, "[UNIC-RX]: UNI BW TLV type %X", tlv_type);

  TLV_DECODE_GETC (bw_tlv_len);

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_err (ZG, "[UNIC-RX]: UNI BW TLV Len %d", bw_tlv_len);

  if (bw_tlv_len < ELMI_BW_PROFILE_FIELD_LENGTH)
    {
      port->stats.inval_mandatory_ie++;
      return RESULT_ERROR;
    }

  TLV_DECODE_GETC (tlv_val);

  per_cos = (tlv_val & 0x01);

  if (per_cos == PAL_TRUE)
    {
      /* per cos bit set wrongly, ignore */
      zlog_err (ZG, "[UNIC-RX]: UNI BW per cos bit set to 1");

      port->stats.inval_mandatory_ie++;

      return RESULT_ERROR;
    }

  uni_bw = uni_info->uni_bw;
  if (!uni_bw)
    {
      uni_bw = XCALLOC (MTYPE_ELMI_BW, 
          sizeof (struct elmi_bw_profile));

      if (!uni_bw)
        {
          return RESULT_ERROR;
        }

      pal_mem_set (uni_bw, 0, sizeof(struct elmi_bw_profile));
      elmi_if->uni_info->uni_bw = uni_bw;
    }    

  uni_bw->cf = ((tlv_val & ELMI_BW_COUPLING_FLAG_MASK) >> 1);
  uni_bw->cm = ((tlv_val & ELMI_BW_COLOR_MODE_MASK) >> 2);

  uni_bw->per_cos = per_cos;

  TLV_DECODE_GETC (uni_bw->cir_magnitude);

  TLV_DECODE_GETW (uni_bw->cir_multiplier);

  uni_bw->cir = 
      elmi_get_number_from_exponent (uni_bw->cir_multiplier,
                                     uni_bw->cir_magnitude);
  
  TLV_DECODE_GETC (uni_bw->cbs_magnitude);

  TLV_DECODE_GETC (uni_bw->cbs_multiplier);

  uni_bw->cbs = 
      elmi_get_number_from_exponent (uni_bw->cbs_multiplier,
                                     uni_bw->cbs_magnitude);

  TLV_DECODE_GETC (uni_bw->eir_magnitude);

  TLV_DECODE_GETW (uni_bw->eir_multiplier);

  uni_bw->eir = elmi_get_number_from_exponent (uni_bw->eir_multiplier,
                                               uni_bw->eir_magnitude);

  TLV_DECODE_GETC (uni_bw->ebs_magnitude);

  TLV_DECODE_GETC (uni_bw->ebs_multiplier);

  uni_bw->ebs = elmi_get_number_from_exponent (uni_bw->ebs_multiplier,
                                               uni_bw->ebs_magnitude);

  TLV_DECODE_GETC (usr_pri);

  if (ELMI_DEBUG(packet, RX)) 
    {
      zlog_info (ZG, "UNI BW profile CIR Mag = %d",
          uni_bw->cir_magnitude);

      zlog_info (ZG, "UNI BW profile CIR Mul = %d", 
          uni_bw->cir_multiplier);

      zlog_info (ZG, "UNI BW profile CBS Mag = %d", 
          uni_bw->cbs_magnitude);

      zlog_info (ZG, "UNI BW profile CBS Mul = %d", 
          uni_bw->cbs_multiplier);

      zlog_info (ZG, "UNI BW profile EIR Mag = %d", 
          uni_bw->eir_magnitude);

      zlog_info (ZG, "UNI BW profile EIR Mul = %d", 
          uni_bw->eir_multiplier);

      zlog_info (ZG, "UNI BW profile EBS Mag = %d", 
          uni_bw->ebs_magnitude);

      zlog_info (ZG, "UNI BW profile EBS Mul = %d", 
          uni_bw->ebs_multiplier);

      zlog_info (ZG, "UNI BW profile user priority = %d", usr_pri);
    }

  return RESULT_OK;

}

/* Decode evc bw Info */
s_int32_t
elmi_decode_evc_bw_profile (u_char **pnt, s_int32_t *size, 
                            u_int32_t buf_len,
                            struct elmi_port *port, 
                            struct elmi_evc_status *evc_info) 
{
  struct elmi_master *em = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_bw_profile *evc_bw = NULL;
  struct elmi_bw_profile *evc_cos_bw = NULL;
  struct elmi_bw_profile tmp_evc_cos_bw;
  u_int8_t tlv_type = 0;
  u_int8_t tlv_length = 0;
  u_int8_t tlv_val = 0;
  u_int8_t per_cos = 0;
  u_int8_t j = 0;
  u_int8_t usr_pri = 0;
  int evc_cos_bw_created = ELMI_FALSE;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em  = elmi_if->elmim;

  if (*size < ELMI_TLV_MIN_SIZE)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: No more bytes left in the frame");

      return RESULT_ERROR;
    }

  /* Decode bandwidth profile parameters */
  TLV_DECODE_GETC (tlv_type);

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_err (ZG, "[UNIC-RX]: Received BW tlv type = %d", tlv_type);

  if (tlv_type != ELMI_BW_PROFILE_SUB_IE_TLV)
    {
      if (tlv_type == ELMI_EVC_ID_SUB_IE_TLV)
        {
          /* Check if any IE is repeated, ignore it */
          TLV_DECODE_GETC (tlv_length);

          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: Received duplicate EVC ID TLV ignore");

          port->stats.duplicate_ie++;

          /* skip the val by length */
          TLV_DECODE_SKIP (tlv_length);
          TLV_DECODE_GETC (tlv_type);

          if (tlv_type != ELMI_BW_PROFILE_SUB_IE_TLV)
            {
              port->stats.mandatory_ie_missing++;
              return RESULT_ERROR;
            }
        }
      else
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: EVC BW profile not present");

          port->stats.mandatory_ie_missing++;
          return RESULT_ERROR;
        }

    }

  TLV_DECODE_GETC (tlv_length);

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_err (ZG, "[UNIC-RX]: Received BW tlv length = %d", tlv_length);

  if (tlv_length < ELMI_BW_PROFILE_FIELD_LENGTH)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: EVC BW tlv length not valid");
      port->stats.inval_mandatory_ie++;
      return RESULT_ERROR;

    }

  TLV_DECODE_GETC (tlv_val);

  per_cos = (tlv_val & 0x01);
  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_err (ZG, "[UNIC-RX]: EVC BW per cos bit set to %d", per_cos);

  if (per_cos == PAL_FALSE)
    {
      evc_bw = evc_info->evc_bw;
      if (!evc_bw)
        {
          evc_bw = XCALLOC (MTYPE_ELMI_BW, 
              sizeof (struct elmi_bw_profile));

          if (!evc_bw)
            return RESULT_ERROR;

          evc_info->evc_bw = evc_bw;
        } 

      evc_info->bw_profile_level = ELMI_EVC_BW_PROFILE_PER_EVC;

      evc_bw->cf = ((tlv_val & ELMI_BW_COUPLING_FLAG_MASK) >> 1);
      evc_bw->cm = ((tlv_val & ELMI_BW_COLOR_MODE_MASK) >> 2);

      TLV_DECODE_GETC (evc_bw->cir_magnitude);

      TLV_DECODE_GETW (evc_bw->cir_multiplier);

      evc_bw->cir = 
        elmi_get_number_from_exponent (evc_bw->cir_multiplier,
            evc_bw->cir_magnitude);

      TLV_DECODE_GETC (evc_bw->cbs_magnitude);

      TLV_DECODE_GETC (evc_bw->cbs_multiplier);

      evc_bw->cbs =
        elmi_get_number_from_exponent (evc_bw->cbs_multiplier,
            evc_bw->cbs_magnitude);

      TLV_DECODE_GETC (evc_bw->eir_magnitude);

      TLV_DECODE_GETW (evc_bw->eir_multiplier);

      evc_bw->eir =
        elmi_get_number_from_exponent (evc_bw->eir_multiplier,
            evc_bw->eir_magnitude);

      TLV_DECODE_GETC (evc_bw->ebs_magnitude);

      TLV_DECODE_GETC (evc_bw->ebs_multiplier);

      evc_bw->ebs =
        elmi_get_number_from_exponent (evc_bw->ebs_multiplier,
            evc_bw->ebs_magnitude);

      TLV_DECODE_GETC (usr_pri);

      if (ELMI_DEBUG(packet, RX)) 
        {
          zlog_info (ZG, "EVC BW profile CIR Mag = %d",
              evc_bw->cir_magnitude);

          zlog_info (ZG, "EVC BW profile CIR Mul = %d", 
              evc_bw->cir_multiplier);

          zlog_info (ZG, "EVC BW profile CBS Mag = %d", 
              evc_bw->cbs_magnitude);

          zlog_info (ZG, "EVC BW profile CBS Mul = %d", 
              evc_bw->cbs_multiplier);

          zlog_info (ZG, "EVC BW profile EIR Mag = %d", 
              evc_bw->eir_magnitude);

          zlog_info (ZG, "EVC BW profile EIR Mul = %d", 
              evc_bw->eir_multiplier);

          zlog_info (ZG, "EVC BW profile EBS Mag = %d", 
              evc_bw->ebs_magnitude);

          zlog_info (ZG, "EVC BW profile EBS Mul = %d", 
              evc_bw->ebs_multiplier);

          zlog_info (ZG, "EVC BW profile user priority = %d", 
              usr_pri);
        }
    }
  else
    {
      /* Maximum 8 bandwidth profile tlv can be present in each EVC */
      while (tlv_type == ELMI_BW_PROFILE_SUB_IE_TLV  && *size)
        {
          if (ELMI_DEBUG(packet, RX)) 
            zlog_info (ZG, "EVC BW profile LEN = %d", tlv_length);

          evc_info->bw_profile_level = ELMI_EVC_BW_PROFILE_PER_COS;

          tmp_evc_cos_bw.cf = ((tlv_val & ELMI_BW_COUPLING_FLAG_MASK) >> 1);
          tmp_evc_cos_bw.cm = ((tlv_val & ELMI_BW_COLOR_MODE_MASK) >> 2);
          tmp_evc_cos_bw.per_cos = per_cos;

          TLV_DECODE_GETC (tmp_evc_cos_bw.cir_magnitude);

          TLV_DECODE_GETW (tmp_evc_cos_bw.cir_multiplier);

          tmp_evc_cos_bw.cir =
            elmi_get_number_from_exponent (tmp_evc_cos_bw.cir_multiplier,
                tmp_evc_cos_bw.cir_magnitude);

          TLV_DECODE_GETC (tmp_evc_cos_bw.cbs_magnitude);

          TLV_DECODE_GETC (tmp_evc_cos_bw.cbs_multiplier);

          tmp_evc_cos_bw.cbs =
            elmi_get_number_from_exponent (tmp_evc_cos_bw.cbs_multiplier,
                tmp_evc_cos_bw.cbs_magnitude);

          TLV_DECODE_GETC (tmp_evc_cos_bw.eir_magnitude);

          TLV_DECODE_GETW (tmp_evc_cos_bw.eir_multiplier);

          tmp_evc_cos_bw.eir =
            elmi_get_number_from_exponent (tmp_evc_cos_bw.eir_multiplier,
                tmp_evc_cos_bw.eir_magnitude);

          TLV_DECODE_GETC (tmp_evc_cos_bw.ebs_magnitude);

          TLV_DECODE_GETC (tmp_evc_cos_bw.ebs_multiplier);

          tmp_evc_cos_bw.ebs =
            elmi_get_number_from_exponent (tmp_evc_cos_bw.ebs_multiplier,
                tmp_evc_cos_bw.ebs_magnitude);

          TLV_DECODE_GETC (tmp_evc_cos_bw.per_cos);

          for (j = 0; j < ELMI_MAX_COS_ID; j++)
            { 
              if (evc_info->cos_val_set[j] == PAL_FALSE)
                {
                  evc_info->cos_val_set[j] = PAL_TRUE; 
                  break; 
                }
            }

          evc_cos_bw = evc_info->evc_cos_bw[j];
          if (evc_cos_bw == NULL)
            {
              evc_cos_bw = XCALLOC (MTYPE_ELMI_BW,
                  sizeof (struct elmi_bw_profile));

              if (!evc_cos_bw)
                {
                  return RESULT_ERROR;
                }
              evc_cos_bw_created = ELMI_TRUE;
            }

          evc_cos_bw->cir = tmp_evc_cos_bw.cir;
          evc_cos_bw->cbs = tmp_evc_cos_bw.cbs;
          evc_cos_bw->eir = tmp_evc_cos_bw.eir;
          evc_cos_bw->ebs = tmp_evc_cos_bw.ebs;
          evc_cos_bw->cf = tmp_evc_cos_bw.cf;
          evc_cos_bw->cm = tmp_evc_cos_bw.cm;
          evc_cos_bw->per_cos = tmp_evc_cos_bw.per_cos;

          pal_mem_set (&tmp_evc_cos_bw, 0, sizeof(struct elmi_bw_profile));

          if (ELMI_DEBUG(packet, RX)) 
            {
              zlog_info (ZG, "EVC COS BW profile CIR Mag = %d",
                  evc_cos_bw->cir_magnitude);

              zlog_info (ZG, "EVC COS BW profile CIR Mul = %d", 
                  evc_cos_bw->cir_multiplier);

              zlog_info (ZG, "EVC COS BW profile CBS Mag = %d", 
                  evc_cos_bw->cbs_magnitude);

              zlog_info (ZG, "EVC COS BW profile CBS Mul = %d", 
                  evc_cos_bw->cbs_multiplier);

              zlog_info (ZG, "EVC COS BW profile EIR Mag = %d", 
                  evc_cos_bw->eir_magnitude);

              zlog_info (ZG, "EVC COS BW profile EIR Mul = %d", 
                  evc_cos_bw->eir_multiplier);

              zlog_info (ZG, "EVC COS BW profile EBS Mag = %d", 
                  evc_cos_bw->ebs_magnitude);

              zlog_info (ZG, "EVC COS BW profile EBS Mul = %d", 
                  evc_cos_bw->ebs_multiplier);

              zlog_info (ZG, "EVC COS BW profile user priority = %d", 
                  evc_cos_bw->per_cos);
            }

          TLV_DECODE_GETC (tlv_type);

          if (tlv_type != ELMI_BW_PROFILE_SUB_IE_TLV)
            {
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: End of EVC COS BW TLV");

              TLV_DECODE_REWIND (ELMI_TLV_LEN);
              break;
            }

          TLV_DECODE_GETC (tlv_length);

          if (tlv_length < ELMI_BW_PROFILE_FIELD_LENGTH)
            {
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: EVC BW tlv length not valid");

              port->stats.inval_mandatory_ie++;
              return RESULT_ERROR;
            }

          TLV_DECODE_GETC (tlv_val);

          per_cos = (tlv_val & 0x01);

          if (evc_cos_bw_created)
            {
              XFREE (MTYPE_ELMI_BW, evc_cos_bw);
              evc_cos_bw_created = ELMI_FALSE;
            }
        }
    }

  return RESULT_OK;
}

/* Decode Uni statue IE */
s_int32_t
elmi_decode_uni_status (u_char **pnt, s_int32_t *size, 
                        u_int32_t buf_len,
                        struct elmi_port *port)
{
  struct elmi_uni_info *uni_status = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;
  u_int8_t tlv_type = 0;
  s_int8_t tlv_length = 0;
  s_int8_t uni_status_tlv_len = 0;
  u_int8_t tlv_val = 0;
  s_int32_t ret = 0;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  if (*size < ELMI_TLV_MIN_SIZE)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: No more bytes left in the frame");

     return RESULT_ERROR;
    }

  /* Parse sequence number type TLV */
  TLV_DECODE_GETC (tlv_type);

  if (tlv_type != ELMI_UNI_STATUS_IE_TLV)
    {
      if (port->last_request_type == ELMI_REPORT_TYPE_FULL_STATUS_CONTD)
        {
          zlog_err (ZG, "[UNIC-RX]: - UNI Tlv not exist, valid tlv");
          TLV_DECODE_REWIND (ELMI_TLV_LEN);
          return RESULT_OK;
        }
      else
        {
          if (tlv_type == ELMI_DATA_INSTANCE_IE_TLV)
            {
              port->stats.duplicate_ie++;
              /* Check if any IE is repeated, ignore it */
              TLV_DECODE_GETC (tlv_length);

              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: Received duplicate TLV ignore");

              /* skip the val by length */
              TLV_DECODE_SKIP (tlv_length);

              TLV_DECODE_GETC (tlv_val);

              if (tlv_val != ELMI_UNI_STATUS_IE_TLV)
                {
                  port->stats.unexpected_ie++;
                  return RESULT_ERROR;
                }
            }
          else
            {
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: Received Invalid TLV");

              port->stats.mandatory_ie_missing++;

              return RESULT_ERROR;
            }
        }
    }

  /* Parse UNI status length */
    TLV_DECODE_GETC (uni_status_tlv_len);

    if (!uni_status_tlv_len)
      {
        if (ELMI_DEBUG(protocol, PROTOCOL))
          zlog_err (ZG, "[UNIC-RX]: UNI status tlv length is zero\n");

        port->stats.inval_status_resp++;

        return RESULT_ERROR;
      }

    TLV_DECODE_GETC (tlv_val);

    uni_status = elmi_if->uni_info;

    /* Allocate memory for UNI and update */
    if (!uni_status)
      {
        elmi_if->uni_info = 
               XCALLOC (MTYPE_ELMI_UNI, sizeof (struct elmi_uni_info));

        if (!elmi_if->uni_info)
          return RESULT_ERROR;  

        pal_mem_set (elmi_if->uni_info, 0, sizeof (struct elmi_uni_info));

        uni_status = elmi_if->uni_info;
      }

    /* validate map type */
    if (tlv_val < ELMI_ALL_TO_ONE_BUNDLING || 
        tlv_val > ELMI_BUNDLING)
      {
        if (ELMI_DEBUG(protocol, PROTOCOL))
          zlog_err (ZG, "[UNIC-RX]: Unknown UNI Map type Received");

        port->stats.mandatory_ie_missing++;
        return RESULT_ERROR;
      }

    uni_status->cevlan_evc_map_type = tlv_val;
    uni_status_tlv_len = uni_status_tlv_len - sizeof(unsigned char);

    if (uni_status_tlv_len > 0 && *size)
      {
        /* Get UNI ID sub ie */
        TLV_DECODE_GETC (tlv_type);

        if (tlv_type != ELMI_UNI_ID_SUB_IE_TLV)
          {
            if (ELMI_DEBUG(protocol, PROTOCOL))
              zlog_err (ZG, "[UNIC-RX]: UNI id not present");

            port->stats.mandatory_ie_missing++;

            return RESULT_ERROR;
          }

        pal_mem_set (uni_status->uni_id, 0, ELMI_UNI_NAME_LEN);

        TLV_DECODE_GETC (tlv_length);
        if (!tlv_length)
          {
            if (ELMI_DEBUG (packet, RX))
              zlog_err (ZG, "[UNIC-RX]: NULL UNI id");
          }

        /* reset previous uni id */
        pal_mem_set (uni_status->uni_id, 0, ELMI_UNI_NAME_LEN);

        TLV_DECODE_GET (uni_status->uni_id, tlv_length);
      }

    uni_status_tlv_len -= (tlv_length + ELMI_TLV_MIN_SIZE);

    if (uni_status_tlv_len > 0) 
      {
        ret = elmi_decode_uni_bw_profile (pnt, size, uni_status_tlv_len, 
                                          port,
                                          uni_status);
        if (ret < 0)
          return RESULT_ERROR;
      }
    else
      {
        if (ELMI_DEBUG (packet, RX))
          zlog_err (ZG, "[UNIC-RX]: UNI BW profile not present");
        return RESULT_ERROR;

      }

    return RESULT_OK;
}

/* Decode Evc Staus Info */
s_int32_t
elmi_decode_evc_status (u_char **pnt, s_int32_t *size, 
                        u_int32_t buf_len,
                        struct elmi_port *port, 
                        u_int8_t report_type)
{
  u_int8_t tlv_type = 0;
  u_int8_t tlv_length = 0;
  u_int8_t tlv_val = 0;
  u_int16_t pre_evc_ref_id = 0;
  struct elmi_evc_status *evc_info = NULL;
  struct elmi_evc_status temp_evc;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;
  s_int32_t ret = 0;
  u_int16_t last_evc_id = 0;
 
  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  pal_mem_set (&temp_evc, 0, sizeof (struct elmi_evc_status));

  if (*size < ELMI_TLV_MIN_SIZE)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: No more bytes left in the frame");

      return RESULT_ERROR;
   }

  /* encode evc type tlv */
  TLV_DECODE_GETC (tlv_val);

  if (tlv_val != ELMI_EVC_STATUS_IE_TLV)
   {

      zlog_err (ZG, "[UNIC-RX]: EVC status TLV = %X", tlv_val);

     if (tlv_val == ELMI_UNI_STATUS_IE_TLV) 
       {
         if (ELMI_DEBUG(protocol, PROTOCOL))
           zlog_err (ZG, "[UNIC-RX]: UNI status TLV duplciated");

         port->stats.duplicate_ie++;
       }
     else  
       {
         if (tlv_val == ELMI_CEVLAN_EVC_MAP_IE_TLV)
           {
             if (ELMI_DEBUG(protocol, PROTOCOL))
               zlog_info (ZG, "[UNIC-RX]: Next TLV is CE-VLAN EVC MAP TLV");

             if (port->last_request_type == ELMI_REPORT_TYPE_FULL_STATUS) 
               {
                 if (ELMI_DEBUG(protocol, PROTOCOL))
                   zlog_info (ZG, "[UNIC-RX]: EVC Status TLV Missing");

                 port->stats.inval_mandatory_ie++;

                 return RESULT_ERROR;
               }
             else
               {
                 TLV_DECODE_REWIND (ELMI_TLV_LEN);
                 return RESULT_OK;
               }
           }
         else
           {
             if (ELMI_DEBUG(protocol, PROTOCOL))
               zlog_err (ZG, "[UNIC-RX]: EVC Status is not Present");

             /* If evcs are not configured, evc tlv may not be present */
             return ELMI_OK;
           }
       }
   }

  while (tlv_val == ELMI_EVC_STATUS_IE_TLV && *size)  
    {
      zlog_err (ZG, "[UNIC-RX]: EVC status decoding function"); 

      TLV_DECODE_GETC (tlv_length);
      if (!tlv_length)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: EVC status length is zero");

          port->stats.inval_mandatory_ie++;
          return RESULT_ERROR;
        }

      TLV_DECODE_GETW (temp_evc.evc_ref_id);

      if (temp_evc.evc_ref_id < ELMI_VLAN_MIN)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: EVC Ref id is zero");

          port->stats.inval_mandatory_ie++; 
          return RESULT_ERROR;
        }

      if (pre_evc_ref_id > temp_evc.evc_ref_id)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: EVC Ref ids out of order");

          port->stats.outof_seq_ie++;
          return RESULT_ERROR;
        }

      /* Check if this full status msg is continuation of last full status
       * contd msg, then compare evc ids 
       */
      if (port->last_request_type == ELMI_REPORT_TYPE_FULL_STATUS_CONTD)
        { 
          if ((port->last_evc_id) && (port->last_evc_id > temp_evc.evc_ref_id))
            { 
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: EVC ref ids not in order");

              port->stats.outof_seq_ie++; 
              return RESULT_ERROR;
            }
          /* Reset it for parseing of next evcs */
          port->last_evc_id = 0; 
        }

      /* Check if this evc is present in local db or new bit is set */
      evc_info = elmi_evc_look_up (elmi_if, temp_evc.evc_ref_id);

      zlog_err (ZG, "[UNIC-RX]: EVC Ref ID = %d", temp_evc.evc_ref_id);

      /* Decode EVC status type */
      TLV_DECODE_GETC (temp_evc.evc_status_type);

      zlog_err (ZG, "[UNIC-RX]: EVC Status type %d", temp_evc.evc_status_type);

      if (temp_evc.evc_status_type > EVC_STATUS_NEW_PARTIALLY_ACTIVE)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: Invalid EVC Status Type");

          port->stats.inval_mandatory_ie++; 
          return RESULT_ERROR;
        }

      /* section 5.6.9.2 MEF16, 
       * If received info for new evc and Active bit is set 
       */
      if ((temp_evc.evc_status_type == EVC_STATUS_ACTIVE) && (!evc_info))
        port->stats.inval_mandatory_ie++; 

      if ((temp_evc.evc_status_type & 0x1)) 
        {
          /* delete old evc and re add */
          if (evc_info)
            {
              if (elmi_if->evc_status_list)
                listnode_delete (elmi_if->evc_status_list, evc_info);

              elmi_evc_status_free (evc_info);
              evc_info = NULL;
            }

          /* allocate memory for new evc */
          evc_info = elmi_evc_new ();

          if (!evc_info)
            {
              return RESULT_ERROR;
            }
        }
      else
        {
          if (!evc_info)
            {
              /* allocate memory for new evc */
              evc_info = elmi_evc_new ();
              if (!evc_info)
                {
                  return RESULT_ERROR;
                }
            }
        }

      evc_info->evc_ref_id = temp_evc.evc_ref_id;
      evc_info->evc_status_type = temp_evc.evc_status_type;
      evc_info->active = PAL_TRUE;

      /* Decode EVC Parameters sub IE */
      TLV_DECODE_GETC (tlv_type);

      zlog_err (ZG, "[UNIC-RX]: EVC Parameters TLV Type = %X", tlv_type);

      if (tlv_type != ELMI_EVC_PARAMS_SUB_IE_TLV)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: EVC Parameters sub IE missing");

          port->stats.inval_mandatory_ie++; 

          XFREE (MTYPE_ELMI_EVC, evc_info);
          return RESULT_ERROR;
        }

      /* Decode EVC parameters length tlv */
      TLV_DECODE_GETC (tlv_length);

      if (!tlv_length)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: EVC Parameters sub IE len invalid");

          port->stats.inval_mandatory_ie++; 

          XFREE (MTYPE_ELMI_EVC, evc_info);
          return RESULT_ERROR;
        }

      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: EVC Parameters sub IE length = %d",
                  tlv_length);

      TLV_DECODE_GETC (evc_info->evc_type);

      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: EVC Parameters = %d", evc_info->evc_type);

      /* check for valid EVC Type */
      if (evc_info->evc_type != ELMI_PT_PT_EVC && 
          evc_info->evc_type != ELMI_MLPT_MLPT_EVC)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: EVC type not valid");

          port->stats.inval_mandatory_ie++; 
          XFREE (MTYPE_ELMI_EVC, evc_info);
          return RESULT_ERROR;
        }

      if ((evc_info->evc_type == ELMI_PT_PT_EVC) &&
          (evc_info->evc_status_type == EVC_STATUS_PARTIALLY_ACTIVE ||
           evc_info->evc_status_type == EVC_STATUS_NEW_PARTIALLY_ACTIVE))
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: EVC Status type not valid");

          port->stats.inval_mandatory_ie++; 
          XFREE (MTYPE_ELMI_EVC, evc_info);
          return RESULT_ERROR;
        }

      /* Decode EVC ID TLV */
      TLV_DECODE_GETC (tlv_type);

      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: EVC ID tlv type = %X", tlv_type);

      if (tlv_type != ELMI_EVC_ID_SUB_IE_TLV)
        {
          if (tlv_type == ELMI_EVC_PARAMS_SUB_IE_TLV)
            {
              /* Check if any IE is repeated, ignore it */
              TLV_DECODE_GETC (tlv_length);

              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: Received duplicate EVC PARAMS TLV ignore");
              port->stats.duplicate_ie++;

              /* skip the val by length */
              TLV_DECODE_SKIP (tlv_length);
              TLV_DECODE_GETC (tlv_type);

              if (tlv_type != ELMI_EVC_ID_SUB_IE_TLV)
                {
                  port->stats.mandatory_ie_missing++;
                  return RESULT_ERROR;
                }
            }
          else
            {
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: EVC Parameters sub IE missing");

              port->stats.mandatory_ie_missing++;
              XFREE (MTYPE_ELMI_EVC, evc_info);
              return RESULT_ERROR;
            }

        }

      /* Decode EVC ID tlv length */
      TLV_DECODE_GETC (tlv_length);

      if (!tlv_length)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: EVC ID TLV length is zero");
        }

      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: EVC ID TLV length is %d", tlv_length);

      TLV_DECODE_GET (evc_info->evc_id, tlv_length);

      ret = elmi_decode_evc_bw_profile (pnt, size, buf_len, port, evc_info);
      if (ret < 0)
        {
          if (evc_info->evc_bw)
            XFREE (MTYPE_ELMI_BW, evc_info->evc_bw);

          XFREE (MTYPE_ELMI_EVC, evc_info);
          return  RESULT_ERROR;
        }

      listnode_add_sort (elmi_if->evc_status_list, evc_info);

      evc_info->active = PAL_TRUE; 

      /* Used to check for ascending order of evcs */
      pre_evc_ref_id = evc_info->evc_ref_id;

      /* encode for next evc type tlv */
      TLV_DECODE_GETC (tlv_val);

      last_evc_id = evc_info->evc_ref_id;

      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Next EVC status TLV type = %d", tlv_val);

      if (tlv_val != ELMI_EVC_STATUS_IE_TLV)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: Unknown TLV type after EVC TLV");

          TLV_DECODE_REWIND (ELMI_TLV_LEN);
          break;
        }

    }

  if (report_type == ELMI_REPORT_TYPE_FULL_STATUS_CONTD)
    {
      /* Store the last evc ref id parsed in the msg */
      port->last_evc_id = last_evc_id;  
    }
  else
    {
      port->last_evc_id = 0;  
    }

  return RESULT_OK;
}

int
elmi_unic_decode_cevlan_evc_map (u_char **pnt, s_int32_t *size,
                                 struct elmi_port *port,
                                 u_int8_t report_type)
{
  u_int8_t tlv_type = 0;
  u_int8_t tlv_length = 0;
  u_int16_t ce_vlan_ref_id = 0;
  u_int16_t previous_evc_ref_id = 0;
  u_int8_t ce_vlan_map_seq = 0;
  u_int8_t ce_vlan_def_evc = 0;
  struct elmi_cvlan_evc_map *cevlan_evc_map = NULL;
  struct elmi_cvlan_evc_map *pre_cevlan_evc_map = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;
  struct elmi_evc_status *evc_info = NULL;
  u_int16_t cvlan = 0;
  u_int8_t default_evc_flag = PAL_FALSE;
  u_int8_t pre_seq_num = PAL_FALSE;
  u_int8_t seq_num = PAL_FALSE;
  u_int8_t last_ie = PAL_FALSE;
  struct elmi_uni_info *uni_info = NULL;
  u_int16_t cvid_count = 0; 
  s_int32_t i = 0; 

  if (!port || !port->elmi_if)
     return RESULT_ERROR;

  elmi_if = port->elmi_if;

  /* Decode CE_VLAN EVC Map IE */
  TLV_DECODE_GETC (tlv_type);

  em =  elmi_if->elmim;

  if (ELMI_DEBUG(protocol, PROTOCOL))
    {
      zlog_err (ZG, "[UNIC-RX]: Start of Decoding for CE-VLAN EVC map TLV");
      zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC map type = %d", tlv_type);
    }

  if (tlv_type != ELMI_CEVLAN_EVC_MAP_IE_TLV)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC map not present");

      port->stats.mandatory_ie_missing++;

      /* Memory will be freed in parent function per now */
      return RESULT_ERROR;
    }

  while (tlv_type == ELMI_CEVLAN_EVC_MAP_IE_TLV)
    {

      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Decoding CE-VLAN EVC map TLVs");

      /* Decode CE_VLAN EVC Map IE */
      TLV_DECODE_GETC (tlv_length);

      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC map len = %d", tlv_length);

      if (!tlv_length)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC map length is zero");

          port->stats.inval_mandatory_ie++;
          return RESULT_ERROR;
        }

      /* Decode CE_VLAN EVC Reference Id */
      TLV_DECODE_GETW (ce_vlan_ref_id);

      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC Ref Id = %d", ce_vlan_ref_id);

      if ((previous_evc_ref_id != 0) &&
          (ce_vlan_ref_id < previous_evc_ref_id))
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: Out of sequence IEs");

          port->stats.outof_seq_ie++; 

          return RESULT_ERROR;
        }
      else if ((previous_evc_ref_id != 0) &&
          (previous_evc_ref_id == ce_vlan_ref_id))
        {
          /* check last ie of previous cevlan-evc map ie */
          pre_cevlan_evc_map = elmi_lookup_cevlan_evc_map (elmi_if, 
              previous_evc_ref_id);
          if (pre_cevlan_evc_map)
            {
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: Last IE of PRE map is %d",
                          pre_cevlan_evc_map->last_ie);

              if (pre_cevlan_evc_map->last_ie == PAL_TRUE)
                {
                  if (ELMI_DEBUG(protocol, PROTOCOL))
                    zlog_err (ZG, "[UNIC-RX]: Last IE of Cevlan-EVC map is"
                        " not set correctly");
                  port->stats.inval_mandatory_ie++; 
                  return RESULT_ERROR;
                }

            }
        }

      /* check if the evc status ie present for this cevlan */
      evc_info = elmi_evc_look_up (elmi_if, ce_vlan_ref_id);
      if (!evc_info)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC ref id does not match EVC");

          port->stats.inval_mandatory_ie++; 

          return RESULT_ERROR;
        }

      /* if the cevlan_evc map exists for the same evc ref id */
      cevlan_evc_map = elmi_lookup_cevlan_evc_map (elmi_if, ce_vlan_ref_id);
      if (!cevlan_evc_map)
        {
          cevlan_evc_map = new_cevlan_evc_map ();
          if (!cevlan_evc_map)
            {
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]:"
                    " Failed to create new CE-VLAN EVC map");

              return RESULT_ERROR;
            }
        }

      cevlan_evc_map->evc_ref_id = ce_vlan_ref_id;

      /* Decode CE_VLAN EVC map seq and last ie */
      TLV_DECODE_GETC (ce_vlan_map_seq);

      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC seq number and last LE = %X",
            ce_vlan_map_seq);

      seq_num = (ce_vlan_map_seq & ELMI_SEQ_NUM_BIT_MASK);  

      last_ie = 
        ((ce_vlan_map_seq & ELMI_LAST_IE_BIT_MASK) >> ELMI_SEQ_NUM_BITS);

      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC seq number = %d Last LE = %d",
            seq_num, last_ie);

      if (pre_cevlan_evc_map)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: Last IE of previous "
                "Ce-vlan EVC map %d", pre_cevlan_evc_map->last_ie);

          if ((previous_evc_ref_id == ce_vlan_ref_id))
            {
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: Cevlan-EVC map Repeated");

              /* check last ie of previous cevlan-evc map */
              if (pre_cevlan_evc_map->last_ie == PAL_TRUE)
                {
                  if (ELMI_DEBUG(protocol, PROTOCOL))
                    zlog_err (ZG, "[UNIC-RX]: Last IE of Previous Cevlan-EVC map"
                        " not set correctly");

                  port->stats.inval_non_mandatory_ie++;
                  if (cevlan_evc_map)
                    XFREE(MTYPE_ELMI_CEVLAN_EVC, cevlan_evc_map);

                  return RESULT_ERROR;
                }
              /* check seq number */
              pre_seq_num = pre_cevlan_evc_map->cvlan_evc_map_seq;

              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: Sequence num of Previous cev-vlan/evc"
                          "  map %d", pre_seq_num);

              if (pre_seq_num >= seq_num)
                {
                  if (ELMI_DEBUG(protocol, PROTOCOL))
                    zlog_err (ZG, "[UNIC-RX]: Sequence number of "
                        " Cevlan-EVC map not set correctly");

                  port->stats.inval_mandatory_ie++;

                  if (cevlan_evc_map)
                    XFREE(MTYPE_ELMI_CEVLAN_EVC, cevlan_evc_map);
                  return RESULT_ERROR;
                }
            }
          else 
            {
              /* Check if the last ie was not set wrongly */
              if (pre_cevlan_evc_map->last_ie == PAL_FALSE)
                {
                  if (ELMI_DEBUG(protocol, PROTOCOL))
                    zlog_err (ZG, "[UNIC-RX]: Last IE of Cevlan-EVC %d"
                        "map is not set correctly", 
                        pre_cevlan_evc_map->evc_ref_id);

                  port->stats.inval_non_mandatory_ie++;

                  if (cevlan_evc_map)
                    XFREE(MTYPE_ELMI_CEVLAN_EVC, cevlan_evc_map);

                  return RESULT_ERROR;
                }
            }
        }

      cevlan_evc_map->last_ie = last_ie;
      cevlan_evc_map->cvlan_evc_map_seq = seq_num;

      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Current CE-VLAN/EVC MAP Last ie [%d]" 
                      " Seq num [%d]", cevlan_evc_map->last_ie,
                       cevlan_evc_map->cvlan_evc_map_seq);

      /* Decode CE_VLAN EVC map seq and last ie */
      TLV_DECODE_GETC (ce_vlan_def_evc);

      zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC default evc bits = %X", 
          ce_vlan_def_evc);

      cevlan_evc_map->default_evc = (ce_vlan_def_evc & 0x01);  

      cevlan_evc_map->untag_or_pri_frame = ((ce_vlan_def_evc >> 1) & 0x01);

      uni_info = elmi_if->uni_info;
      if (!uni_info)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC MAP: uni info "
                "not present");

          if (cevlan_evc_map)
            XFREE(MTYPE_ELMI_CEVLAN_EVC, cevlan_evc_map);
          return RESULT_ERROR;
        }

      if (cevlan_evc_map->default_evc)
        {
          /* As per setion of 5.5.3.5 of MEF16, deafult evc 
           * should be set only for one evc*/
          if (default_evc_flag == PAL_TRUE)
            {
              if (previous_evc_ref_id != ce_vlan_ref_id)
                {
                  if (ELMI_DEBUG(protocol, PROTOCOL))
                    zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC default evc bit "
                        "set for more than one EVC");

                  port->stats.inval_non_mandatory_ie++;

                  if (cevlan_evc_map)
                    XFREE(MTYPE_ELMI_CEVLAN_EVC, cevlan_evc_map);
                  return RESULT_ERROR;
                }
            }

          /* Check the map type as default evc bit is valid only 
           * for map type bundling
           */
          if (uni_info->cevlan_evc_map_type != ELMI_BUNDLING)
            {
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC default evc bit "
                    "set for invalid map type");

              port->stats.inval_mandatory_ie++;

              if (cevlan_evc_map)
                XFREE(MTYPE_ELMI_CEVLAN_EVC, cevlan_evc_map);
              return RESULT_ERROR;
            }

          default_evc_flag = PAL_TRUE;
        }

      if (cevlan_evc_map->untag_or_pri_frame && 
          uni_info->cevlan_evc_map_type == ELMI_ALL_TO_ONE_BUNDLING)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC MAP: Untagged/Priority"
                "tagged bit set for invalid map type");

          port->stats.inval_non_mandatory_ie++;

          if (cevlan_evc_map)
            XFREE(MTYPE_ELMI_CEVLAN_EVC, cevlan_evc_map);
          return RESULT_ERROR;
        }

      /* Decode CE_VALN ID/EVC MAP sub information */
      TLV_DECODE_GETC (tlv_type);

      /* Decode ce-vlan membership tlv length */
      TLV_DECODE_GETC (tlv_length);

      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC membership len = %d", 
            tlv_length);

      if (!tlv_length)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC map membership not present");

          port->stats.inval_non_mandatory_ie++; 

          if (cevlan_evc_map)
            XFREE(MTYPE_ELMI_CEVLAN_EVC, cevlan_evc_map);

          return RESULT_ERROR;
        }

      cvid_count = tlv_length/2;

      for (i = 0; i < cvid_count && tlv_length; i++)
        {
          TLV_DECODE_GETW (cvlan);
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: CE-VLAN EVC Cvlan %d", cvlan);

          tlv_length = (tlv_length - sizeof(u_int16_t));

          /* Add cvlan to cvlan membership */
          ELMI_VLAN_BMP_SET (cevlan_evc_map->cvlanbitmap, cvlan);
        }

      /* validate cevlan membership set */
      if (pre_cevlan_evc_map && previous_evc_ref_id != ce_vlan_ref_id)
        {
          /* get each cvlan mapped to evc */
          ELMI_VLAN_SET_BMP_ITER_BEGIN (cevlan_evc_map->cvlanbitmap, cvlan)
            {
              if ((ELMI_VLAN_BMP_IS_MEMBER (&pre_cevlan_evc_map->cvlanbitmap,
                      cvlan)))
                {
                  if (ELMI_DEBUG(protocol, PROTOCOL))
                    zlog_err (ZG, "[UNIC-RX]: CE-VLAN %d "
                        "is member of more than 1 EVC");

                  port->stats.inval_non_mandatory_ie++;

                  if (cevlan_evc_map)
                    XFREE(MTYPE_ELMI_CEVLAN_EVC, cevlan_evc_map);

                  return RESULT_ERROR;
                }
            }
          ELMI_VLAN_SET_BMP_ITER_END (cevlan_evc_map->cvlanbitmap, cvlan);
        }

      zlog_err (ZG, "[UNIC-RX]:CE-VLAN EVC LAST IE %d", 
          cevlan_evc_map->last_ie);

      listnode_add_sort (elmi_if->cevlan_evc_map_list, cevlan_evc_map);

      /* encode for next cevlan evc map tlv */
      TLV_DECODE_GETC (tlv_type);

      previous_evc_ref_id = ce_vlan_ref_id;
    }

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_err (ZG, "[UNIC-RX]: END of CE-VLAN EVC map decoding");

  return RESULT_OK;

}

/* Parse Status pdu (ELMI Check msg) */
s_int32_t
elmi_unic_parse_status_pdu (u_char **pnt, s_int32_t *size,
                            u_int32_t buf_len,
                            struct elmi_port *port)
{
  
  struct elmi_master *em = NULL;
  struct elmi_ifp *elmi_if = NULL;
  u_int32_t data_instance = 0;
  u_int8_t seq_num = 0;
  u_int8_t tlv_type = 0;
  s_int32_t ret = 0;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  /* Decode the common part */
  ret = elmi_decode_seq_and_data_instance (pnt, size, buf_len, port,
                                           &data_instance, &seq_num);
  if (ret < 0)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: - Error in parsing ELMI check msg");

      return RESULT_ERROR;
    }

  /* check if any other tlvs also present */ 
  if (*size)
    {
      TLV_DECODE_GETC (tlv_type); 
      if (tlv_type)
        { 
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: - Unrecognized tlv present");

          port->stats.unexpected_ie++;
        }
     }

  port->recvd_send_seq_num = seq_num;

  port->rcvd_response = PAL_TRUE; 
  
  port->stats.last_status_check_rcvd = pal_time_current (0);

  if (port->data_instance != data_instance)
    {
      elmi_uni_c_tx (port, ELMI_REPORT_TYPE_FULL_STATUS);

      /* This timer can be updated in tx also */
      port->stats.last_full_status_enq_sent = pal_time_current (0);
    }

  return ret;

}

/* Parse Full status pdu */
s_int32_t
elmi_unic_parse_full_status_pdu (u_char **pnt, s_int32_t *size, 
                                 u_int32_t buf_len, 
                                 struct elmi_port *port)
{
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;
  struct listnode *node  = NULL;
  struct listnode *next  = NULL;
  struct elmi_evc_status *evc_node = NULL;
  struct elmi_cvlan_evc_map *cevlan_evc_map = NULL;
  u_int32_t data_instance = 0;
  u_int8_t seq_num = 0;
  s_int32_t ret = 0;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_debug (ZG, "[UNIC-RX]: Full status rcvd on port %d", 
                elmi_if->ifindex);

  /* Decode the common part */
   ret = elmi_decode_seq_and_data_instance (pnt, size, buf_len, port, 
                                  &data_instance, &seq_num);
   if (ret < 0) 
     return RESULT_ERROR;

   /* If last msg was full status contd msg, then no UNI info */

   /* Decode UNI Status */
   ret = elmi_decode_uni_status (pnt, size, buf_len, port);
   if (ret < 0) 
     {
       if (ELMI_DEBUG(protocol, PROTOCOL))
         zlog_err (ZG, "[UNIC-RX]: - Error in UNI status decoding");

       elmi_unic_clear_learnt_database (port->elmi_if);
       return RESULT_ERROR;
     }

   /* Decode EVC Status */
   ret = elmi_decode_evc_status (pnt, size, buf_len, port, 
                                 ELMI_REPORT_TYPE_FULL_STATUS);
   if (ret < 0) 
     {
       if (ELMI_DEBUG(protocol, PROTOCOL))
         zlog_err (ZG, "[UNIC-RX]: - Error in EVC status decoding");

       /* Clear database and free memory */
       elmi_unic_clear_learnt_database (port->elmi_if);

       return RESULT_ERROR;
     }
   else if (ret != ELMI_OK)
     {
       if (*size <= 0)
         {
           if (ELMI_DEBUG(protocol, PROTOCOL))
             zlog_err (ZG, "[UNIC-RX]: CE_VLAN/EVC MAP Not present");

           /* Clear database and free memory */
           elmi_unic_clear_learnt_database (port->elmi_if);

           return RESULT_ERROR;
         }

       /* Decode CE_VLAN/EVC MAP Status TLV */
       ret = elmi_unic_decode_cevlan_evc_map (pnt, size, port,
           ELMI_REPORT_TYPE_FULL_STATUS);
       if (ret < 0) 
         {
           if (ELMI_DEBUG(protocol, PROTOCOL))
             zlog_err (ZG, "[UNIC-RX]: - Error in CE_VLAN/EVC MAP decoding");

           /* Clear database and free memory */
           elmi_unic_clear_learnt_database (port->elmi_if);

           return RESULT_ERROR;
         }

     }

   /* Remove the evcs which are not present in the new status msg */
   if (elmi_if->evc_status_list && listcount (elmi_if->evc_status_list) > 0)
     for (node = LISTHEAD (elmi_if->evc_status_list); node; node = next)
       {
         next = node->next;

         evc_node =  (struct elmi_evc_status *)(GETDATA (node)) ;
         if (evc_node && evc_node->active == PAL_FALSE)
           {
             /* delete from old list */
             elmi_unic_delete_evc (elmi_if, evc_node);
             listnode_delete (elmi_if->evc_status_list, evc_node);
             XFREE (MTYPE_ELMI_EVC, evc_node);
           }
         else
           {
             /* check for missing cevlan/evc map */
             cevlan_evc_map = elmi_lookup_cevlan_evc_map (elmi_if,
                 evc_node->evc_ref_id);
             if (!cevlan_evc_map)
               {
                 if (ELMI_DEBUG(protocol, PROTOCOL))
                   zlog_err (ZG, "[UNIC-RX]: - Some CE_VLAN/EVC MAP TLVs missing");

                 /* Clear database and free memory */
                 elmi_unic_clear_learnt_database (port->elmi_if);

                 return RESULT_ERROR;
               }
           }
       }

  /* Update the flag as received response from UNI-N */
  port->rcvd_response = PAL_TRUE;

  /* Save received UNIN Sent seq num */
  port->recvd_send_seq_num = seq_num;

   /* Update data instance field */
  port->data_instance = data_instance;

  /* Check for cevlan creation */
  elmi_validate_and_create_cvlans (port);

  /* Update time stamp */
  port->stats.last_full_status_rcvd = pal_time_current (0);

  return RESULT_OK;

}

/* UNI-C parse full status continued pdu */
s_int32_t
elmi_unic_parse_full_status_contd_pdu (u_char **pnt, s_int32_t *size,
                                       u_int32_t buf_len,
                                       struct elmi_port *port)
{
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;
  u_int32_t data_instance = 0;
  u_int8_t seq_num = 0;
  s_int32_t ret = 0;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_debug (ZG, "[UNIC-RX]: ELMI Frame Recognition -"
                " ELMI Full status rcvd on port %d",
                elmi_if->ifindex);

  /* Decode seq number and data instance tlvs */
  ret = elmi_decode_seq_and_data_instance (pnt, size, buf_len, port,
                                           &data_instance, &seq_num);
  if (ret < 0)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Error in decoding of seq num or " 
                  "data instance");

      return RESULT_ERROR;
    }

  /* Decode UNI Status */
  if (port->last_request_type != ELMI_REPORT_TYPE_FULL_STATUS_CONTD)
    ret = elmi_decode_uni_status (pnt, size, buf_len, port);
  if (ret < 0)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: - Error in UNI status decoding");

      elmi_unic_clear_learnt_database (port->elmi_if);
      return RESULT_ERROR;
    }

  /* Decode EVC Status */
  ret = elmi_decode_evc_status (pnt, size, buf_len, port, 
                                ELMI_REPORT_TYPE_FULL_STATUS_CONTD);
  if (ret < 0)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: - Error in decoding of EVC status");

      return RESULT_ERROR;
    }

  if (ELMI_DEBUG(protocol, PROTOCOL)) 
    zlog_err (ZG, "[UNIC-RX]: Encoding Done for EVC status");

  if (*size)
    {
      /* Decode CE_VLAN/EVC MAP Status TLV */
      ret = elmi_unic_decode_cevlan_evc_map (pnt, size, port,
                             ELMI_REPORT_TYPE_FULL_STATUS_CONTD);

      if (ret < 0)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIC-RX]: - Error in CE_VLAN/EVC MAP decoding");

          /* Clear database and free memory */
          elmi_unic_clear_learnt_database (port->elmi_if);

          return RESULT_ERROR;
        }
    }

  /* Save received Sent seq num from UNI-N */
  port->recvd_send_seq_num = seq_num;

  /* Do not update data instance field for contd frames */
  port->last_request_type = ELMI_REPORT_TYPE_FULL_STATUS_CONTD;

  /* Send again next full status contd msg and restart the timer */
  elmi_uni_c_tx (port, ELMI_REPORT_TYPE_FULL_STATUS_CONTD);

  if (l2_is_timer_running (port->polling_timer))
    l2_stop_timer (&port->polling_timer);

  port->polling_timer = l2_start_timer (elmi_polling_timer_handler, port,
                                        SECS_TO_TICS(elmi_if->polling_time),
                                        elmi_zg);
  return RESULT_OK;

}

/* Parse single EVC Async message */
s_int32_t
elmi_unic_parse_async_evc_pdu (u_char **pnt, s_int32_t *size,
                               u_int32_t buf_len, struct elmi_port *port)
{
  struct elmi_evc_status *evc_info = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;
  u_int8_t tlv_length = 0;
  u_int8_t tlv_type = 0;
  u_int16_t evc_ref_id = 0;
  u_int8_t evc_status_type = 0;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  /* Data instance and seq num won't be present */

  /* Parse evc status msg and validate it */
  TLV_DECODE_GETC (tlv_type);

  if (tlv_type != ELMI_EVC_STATUS_IE_TLV)
    {
      if (tlv_type == ELMI_SEQUENCE_NUM_IE_TLV ||
          tlv_type == ELMI_DATA_INSTANCE_IE_TLV ||
          tlv_type == ELMI_UNI_STATUS_IE_TLV)
        {
          port->stats.unexpected_ie++;
          return RESULT_ERROR;

        }
      else
        {
          port->stats.unrecognized_ie++;
          return RESULT_ERROR;
        }
    }

  TLV_DECODE_GETC (tlv_length);
  if (!tlv_length)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: EVC status length is zero");

      port->stats.inval_mandatory_ie++;

      return RESULT_ERROR;
    }

  TLV_DECODE_GETW (evc_ref_id);

  if (evc_ref_id < ELMI_VLAN_MIN)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: EVC Ref id is zero");

      port->stats.inval_mandatory_ie++;
      return RESULT_ERROR;
    }

  /* Decode EVC status type */
  TLV_DECODE_GETC (evc_status_type);

  /* Check if the evc present  */
  evc_info = elmi_evc_look_up (port->elmi_if, evc_ref_id);

  if (!evc_info)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: EVC not found");

      return RESULT_ERROR;        
    }

  /* Update the status in the evc */
  evc_info->evc_status_type = evc_status_type;

  return RESULT_OK;
}

s_int32_t
elmi_unic_handle_frame (struct lib_globals *zg, 
                        struct sockaddr_l2 *l2_skaddr,
                        u_char **pnt, s_int32_t *size)

{
  struct apn_vr *vr = apn_vr_get_privileged (zg);
  struct elmi_port *port = NULL;
  struct elmi_master *em = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct interface *ifp = NULL;
  struct elmi_pdu pdu; 
  u_int16_t buf_len = 0;
  s_int32_t ret = 0;

  /* Get port info */
  ifp = if_lookup_by_index (&vr->ifm, l2_skaddr->port);

  elmi_if = ifp->info;

  if (!elmi_if || !elmi_if->elmi_enabled)
    return RESULT_ERROR;

  port = elmi_if->port;
  if (!port)
    return RESULT_ERROR;

  em = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  pal_mem_set (&pdu, 0, sizeof(struct elmi_pdu));
    
  ret = elmi_unic_validate_rcvd_pdu (elmi_if, pnt, size, l2_skaddr, &pdu);
  if (ret < 0)
    return RESULT_ERROR;

  /* Received response from, so reset the flag */
  if (pdu.report_type != ELMI_REPORT_TYPE_SINGLE_EVC)
    port->request_sent = PAL_FALSE;

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_err (ZG, "[UNIC-RX]: Received Report Type %d", pdu.report_type);

  switch (pdu.report_type)
    {
    case ELMI_REPORT_TYPE_FULL_STATUS:
      ret = elmi_unic_parse_full_status_pdu (pnt, size, buf_len, port); 
      break;
    case ELMI_REPORT_TYPE_ECHECK:
      ret = elmi_unic_parse_status_pdu (pnt, size, buf_len, port); 
      break;
    case ELMI_REPORT_TYPE_SINGLE_EVC:
      ret = elmi_unic_parse_async_evc_pdu (pnt, size, buf_len, port);
      break;
    case ELMI_REPORT_TYPE_FULL_STATUS_CONTD:
      ret = elmi_unic_parse_full_status_contd_pdu (pnt, size, buf_len, port);
      break;
    default:
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Received Invalid Report Type");
      break;
    }

  if (ret < 0)
    {
      return RESULT_ERROR;
    }

  /* Counter to maintain recent N393 success operations */
  port->rcnt_success_oper_cnt++;

  /* Reset status counter */
  port->current_status_counter = ELMI_COUNTER_MIN;

  /* check if the N393 recent responses are successful */
  if (port->rcnt_success_oper_cnt == elmi_if->status_counter_limit)
    {
      /* change ELMI operational state */
      if (port->elmi_operational_state != ELMI_OPERATIONAL_STATE_UP)
        {
          port->elmi_operational_state = ELMI_OPERATIONAL_STATE_UP;

          if (ELMI_DEBUG (protocol, PROTOCOL))
            zlog_debug (ZG, "elmi_polling_timer_handler: "
                "ELMI state changed to Operational");

          /* Send notification to NSM */
          elmi_nsm_send_operational_status (port,
              port->elmi_operational_state);
        }
    }


  return ret;
}

/* Check for cevlan creation */
s_int32_t
elmi_validate_and_create_cvlans (struct elmi_port * port)
{
  struct elmi_cvlan_evc_map *cevlan_evc_map = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;
  struct listnode *node = NULL;
  struct nsm_msg_vlan_port msg;
  u_int16_t cvlan = 0;
  s_int16_t count = 0;
  s_int32_t ret = 0;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if; 

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  LIST_LOOP (elmi_if->cevlan_evc_map_list, cevlan_evc_map, node)
    {
      /* get each cvlan mapped to evc */
      ELMI_VLAN_SET_BMP_ITER_BEGIN (cevlan_evc_map->cvlanbitmap , cvlan) 
        {
          /* For each evc get cvlan membership and do autoconfig if required */
          if (!ELMI_VLAN_BMP_IS_MEMBER (elmi_if->vlan_bmp, cvlan))
            count++;
        }
      ELMI_VLAN_SET_BMP_ITER_END (cevlan_evc_map->cvlanbitmap, cvlan); 

        if (count > 0)
          {
            pal_mem_set (&msg, 0, sizeof (struct nsm_msg_vlan_port));
            msg.ifindex = elmi_if->ifindex;
            msg.num = count;
            msg.vid_info = XCALLOC (MTYPE_TMP, sizeof (u_int16_t) * msg.num);

            count = 0;

            /* get each cvlan mapped to evc */
            ELMI_VLAN_SET_BMP_ITER_BEGIN (cevlan_evc_map->cvlanbitmap , cvlan)
              {
                /* For each evc get cvlan membership and 
                 * do autoconfig if required 
                 */
                if (!ELMI_VLAN_BMP_IS_MEMBER (elmi_if->vlan_bmp, cvlan))
                  { 
                    msg.vid_info[count] = cvlan;
                    count++;
                  }
              }
            ELMI_VLAN_SET_BMP_ITER_END (cevlan_evc_map->cvlanbitmap, cvlan);

            /* Send message to NSM regarding auto config */
            ret = elmi_nsm_send_auto_vlan_port (ZG->nc, &msg, 
                                             NSM_MSG_ELMI_AUTO_VLAN_ADD_PORT);
            
            if (ret < 0)
              {
                if (ELMI_DEBUG(protocol, PROTOCOL))
                  zlog_err (ZG, "[UNIC-RX]:"
                      " Addition of vlan failed in NSM");
              }

            XFREE (MTYPE_TMP, msg.vid_info);

          }
    }

    return  RESULT_OK;

}

/* Internal function to dump the frame */
void
elmi_hexdump (unsigned char *buf, int nbytes)
{
  int i, j;

  for (i = 0; i < nbytes; i += 16) 
    {
      printf("%04X : ", i);
      for (j = 0; j < 16 && i + j < nbytes; j++)
        printf("%2.2X ", buf[i + j]);
      for (; j < 16; j++) {
          printf("   ");
      }
      printf(": ");
      for (j = 0; j < 16 && i + j < nbytes; j++) {
          char c = toascii(buf[i + j]);
          printf("%c", isalnum(c) ? c : '.');
      }
      printf("\n");
    }
      printf("End of frame\n");
}

