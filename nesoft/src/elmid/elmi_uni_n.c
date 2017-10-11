/**@file elmi_uni_n.c
   @brief  This file contains TX and RX state machines and corresponding. 
   encoding and decoding functions for UNIN
 */
/* Copyright (C) 2010  All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "l2lib.h"
#include "thread.h"
#include "timeutil.h"
#include "tlv.h"
#include "avl_tree.h"
#include "l2_debug.h"
#include "elmi_types.h"
#include "elmi_port.h"
#include "elmi_error.h"
#include "elmi_timer.h"
#include "elmid.h"
#include "elmi_sock.h"
#include "elmi_uni_n.h"
#include "elmi_uni_c.h"
#include "elmi_debug.h"

s_int32_t
elmi_cvlan_map_len (struct elmi_cvlan_evc_map *map_info);

/* Get Total EVC Status Len */
s_int32_t
elmi_get_evc_length (struct elmi_port *port, 
                     struct elmi_evc_status *evc_info)
{
  u_int8_t evc_len = 0;
  u_int8_t evc_id_len = 0;
  u_int8_t evc_bw_len = 0;
  u_int8_t cos_id = 0;
  struct elmi_bw_profile *evc_cos_bw = NULL;
  struct elmi_ifp *elmi_if = NULL;

  if (!evc_info || !port || !port->elmi_if)
    return evc_len;

  elmi_if = port->elmi_if;

  evc_id_len = pal_strlen (evc_info->evc_id); 
  if (evc_id_len > ELMI_EVC_NAME_LEN)
    evc_id_len = ELMI_EVC_NAME_LEN;
  else if (evc_id_len == 0)
    evc_id_len = ELMI_TLV_LEN;

  evc_len = evc_id_len + ELMI_EVC_FIXED_FRAME_LEN;

  if (evc_info->bw_profile_level == ELMI_EVC_BW_PROFILE_PER_EVC)
    evc_bw_len = (ELMI_TLV_MIN_SIZE + ELMI_BW_PROFILE_SUB_IE_LEN);
  else if (evc_info->bw_profile_level == ELMI_EVC_BW_PROFILE_PER_COS)
    {
      for (cos_id = 0; cos_id < ELMI_MAX_COS_ID; cos_id++)
        {
          evc_cos_bw = evc_info->evc_cos_bw[cos_id];
          if (evc_cos_bw)
            {
              /* BW tlv Type + BW tlv len + BW tlv val */
              evc_bw_len += ELMI_TLV_MIN_SIZE;
              evc_bw_len += ELMI_BW_PROFILE_SUB_IE_LEN;
            }
        }
    }
  else
    evc_bw_len = (ELMI_TLV_MIN_SIZE + ELMI_BW_PROFILE_SUB_IE_LEN);

  evc_len += evc_bw_len;

  return evc_len;
}

/* Get Total EVC Status Len */
s_int32_t
elmi_cvlan_map_len (struct elmi_cvlan_evc_map *map_info)
{
  u_int16_t cvlan_map_len = 0;
  u_int16_t cvlans_cnt = 0;
  u_int16_t vid = 0;

  ELMI_VLAN_SET_BMP_ITER_BEGIN (map_info->cvlanbitmap, vid)
    {
      cvlans_cnt++;
    }
  ELMI_VLAN_SET_BMP_ITER_END (map_info->cvlanbitmap, vid);

  zlog_info (ZG, "[UNIN-TX]: No of cvlans for EVC %d is %d", 
             map_info->evc_ref_id, cvlans_cnt);

  cvlan_map_len = (cvlans_cnt * 2)+ ELMI_CEVLAN_EVC_MAP_FIXED_LEN;

  return cvlan_map_len;
}

/*******************************************************
  Encoding part for UNI-N TX module
********************************************************/

/* Encode Bandwidth Profile Parameters */
void 
elmi_encode_uni_bw_profile (u_char **pnt, u_int32_t *size,
                            struct elmi_port *port) 

{
  struct elmi_ifp *elmi_if = NULL; 
  struct elmi_bw_profile *uni_bw = NULL; 
  struct elmi_uni_info *uni_info = NULL; 
  u_int8_t per_cos = 0;

  elmi_if = port->elmi_if;

  uni_info = elmi_if->uni_info;
  if (!uni_info)
    return;

  uni_bw = uni_info->uni_bw;

  if (uni_bw && elmi_if->bw_profile_level == ELMI_BW_PROFILE_PER_UNI)
    {
      TLV_ENCODE_PUTC (ELMI_BW_PROFILE_SUB_IE_TLV);
      TLV_ENCODE_PUTC (ELMI_BW_PROFILE_SUB_IE_LEN);

      /* encode per cos bit */
      per_cos = per_cos | (uni_bw->cf << 1);
      per_cos = per_cos | (uni_bw->cm << 2);

      TLV_ENCODE_PUTC (per_cos);

      TLV_ENCODE_PUTC (uni_bw->cir_magnitude);
      TLV_ENCODE_PUTW (uni_bw->cir_multiplier);

      TLV_ENCODE_PUTC (uni_bw->cbs_magnitude);
      TLV_ENCODE_PUTC (uni_bw->cbs_multiplier);

      TLV_ENCODE_PUTC (uni_bw->eir_magnitude);
      TLV_ENCODE_PUTW (uni_bw->eir_multiplier);

      TLV_ENCODE_PUTC (uni_bw->ebs_magnitude);
      TLV_ENCODE_PUTC (uni_bw->ebs_multiplier);
      TLV_ENCODE_PUTC (0);
    }
  else
    {
      TLV_ENCODE_PUTC (ELMI_BW_PROFILE_SUB_IE_TLV);
      TLV_ENCODE_PUTC (ELMI_BW_PROFILE_SUB_IE_LEN);
      TLV_ENCODE_PUTC (per_cos);
      TLV_ENCODE_PUT_EMPTY (ELMI_BW_PROFILE_SUB_IE_LEN-1);
    }

   return;
}

/* Encode Bandwidth Profile Parameters */
void 
elmi_encode_evc_bw_profile (u_char **pnt, u_int32_t *size,
                            struct elmi_port *port, 
                            struct elmi_evc_status *evc_info) 

{
  struct elmi_ifp *elmi_if = NULL; 
  struct elmi_bw_profile *evc_bw = NULL; 
  struct elmi_bw_profile *evc_cos_bw = NULL; 
  struct elmi_master *em = NULL;
  u_int8_t per_cos = 0;
  s_int8_t i = 0;

  if (!port || !port->elmi_if)
    return;

  elmi_if = port->elmi_if;

  em = elmi_if->elmim;

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_info (ZG, "BW profile type = %d", elmi_if->bw_profile_level);

  if (evc_info->bw_profile_level == ELMI_EVC_BW_PROFILE_PER_EVC)
    {
      evc_bw = evc_info->evc_bw;
      if (evc_bw)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_info (ZG, "Encoding EVC BW profile type");

          TLV_ENCODE_PUTC (ELMI_BW_PROFILE_SUB_IE_TLV);
          TLV_ENCODE_PUTC (ELMI_BW_PROFILE_SUB_IE_LEN);

          /* encode per cos bit */
          per_cos = per_cos | (evc_bw->cf << 1);
          per_cos = per_cos | (evc_bw->cm << 2);

          TLV_ENCODE_PUTC (per_cos);

          TLV_ENCODE_PUTC (evc_bw->cir_magnitude);
          TLV_ENCODE_PUTW (evc_bw->cir_multiplier);

          TLV_ENCODE_PUTC (evc_bw->cbs_magnitude);
          TLV_ENCODE_PUTC (evc_bw->cbs_multiplier);

          TLV_ENCODE_PUTC (evc_bw->eir_magnitude);
          TLV_ENCODE_PUTW (evc_bw->eir_multiplier);

          TLV_ENCODE_PUTC (evc_bw->ebs_magnitude);
          TLV_ENCODE_PUTC (evc_bw->ebs_multiplier);

          TLV_ENCODE_PUTC (ELMI_RESERVED_BYTE);
        }
      else
        {
          TLV_ENCODE_PUTC (ELMI_BW_PROFILE_SUB_IE_TLV);
          TLV_ENCODE_PUTC (ELMI_BW_PROFILE_SUB_IE_LEN);
          TLV_ENCODE_PUT_EMPTY (ELMI_BW_PROFILE_SUB_IE_LEN);
        }
    }
  else if (evc_info->bw_profile_level == ELMI_EVC_BW_PROFILE_PER_COS)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_info (ZG, "Encoding EVC COS BW profile type");

      /* get cos id and encode */
      for (i = 0; i < ELMI_MAX_COS_ID; i++)
        {
          evc_cos_bw = evc_info->evc_cos_bw[i];
          if (evc_cos_bw)
            {
              TLV_ENCODE_PUTC (ELMI_BW_PROFILE_SUB_IE_TLV);
              TLV_ENCODE_PUTC (ELMI_BW_PROFILE_SUB_IE_LEN);

              per_cos = 1;

              /* encode per cos bit */
              per_cos = per_cos | (evc_cos_bw->cf << 1);
              per_cos = per_cos | (evc_cos_bw->cm << 2);

              TLV_ENCODE_PUTC (per_cos);

              TLV_ENCODE_PUTC (evc_cos_bw->cir_magnitude);
              TLV_ENCODE_PUTL (evc_cos_bw->cir_multiplier);

              TLV_ENCODE_PUTC (evc_cos_bw->cbs_magnitude);
              TLV_ENCODE_PUTC (evc_cos_bw->cbs_multiplier);

              TLV_ENCODE_PUTC (evc_cos_bw->eir_magnitude);
              TLV_ENCODE_PUTL (evc_cos_bw->eir_multiplier);

              TLV_ENCODE_PUTC (evc_cos_bw->ebs_magnitude);
              TLV_ENCODE_PUTL (evc_cos_bw->ebs_multiplier);

              /* Encode user pri bits */
              TLV_ENCODE_PUTC (evc_cos_bw->per_cos);
            }
        }
    }

  else
    {
      TLV_ENCODE_PUTC (ELMI_BW_PROFILE_SUB_IE_TLV);
      TLV_ENCODE_PUTC (ELMI_BW_PROFILE_SUB_IE_LEN);
      TLV_ENCODE_PUT_EMPTY (ELMI_BW_PROFILE_SUB_IE_LEN);
    }

  return;
}

/* Encode Header: Protocol, Msg Type, Report Type */
void
elmi_unin_encode_header (u_char **pnt, u_int32_t *size, 
                         u_int8_t report_type)
{
  /* u_int32_t tmp_size = *size; */

  TLV_ENCODE_PUTC (ELMI_PROTO_VERSION);

  /* Message Type */
  TLV_ENCODE_PUTC (ELMI_MSG_TYPE_STATUS);

  TLV_ENCODE_PUTC (ELMI_REPORT_TYPE_IE_TLV);

  /* Report type length tlv */
  TLV_ENCODE_PUTC (ELMI_REPORT_TYPE_IE_LENGTH);

  /* Report type val */
  TLV_ENCODE_PUTC (report_type);

  return;

}

/* Encode Common fields: Seq Num and Data instance */
void
elmi_unin_encode_seqnum_and_di (u_char **pnt, u_int32_t *size, 
                                struct elmi_port *port)
{
  /* Sequence Number */
  TLV_ENCODE_PUTC (ELMI_SEQUENCE_NUM_IE_TLV);

  /* Sequence Number length*/
  TLV_ENCODE_PUTC (ELMI_SEQUENCE_NUM_IE_LENGTH);

  if (port->sent_seq_num >= ELMI_SEQ_NUM_MAX)
    port->sent_seq_num = 0;

  /* Send Sequence Number val */
  TLV_ENCODE_PUTC (++port->sent_seq_num);

  /* Received send Sequence Number val */
  TLV_ENCODE_PUTC (port->recvd_send_seq_num);

  /* Data instance tlv type */
  TLV_ENCODE_PUTC (ELMI_DATA_INSTANCE_IE_TLV);
  TLV_ENCODE_PUTC (ELMI_DATA_INSTANCE_IE_LENGTH);

  TLV_ENCODE_PUTC (ELMI_RESERVED_BYTE);

  /* Data instance val tlv */
  TLV_ENCODE_PUTL (port->data_instance);

  return;

}

/* Encode UNI Status IE */
void
elmi_encode_uni_status_tlv (u_char **pnt, u_int32_t *size, 
                            struct elmi_port *port)
{
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_uni_info *uni_info = NULL;
  u_int8_t uni_id_len = 0;
  u_int8_t uni_id_len_null = PAL_FALSE;
  u_int8_t uni_len = 0;

  if (!port || !port->elmi_if)
    return;

  elmi_if = port->elmi_if;

  if (!elmi_if->uni_info)
    return;

  TLV_ENCODE_PUTC (ELMI_UNI_STATUS_IE_TLV);

  uni_info = elmi_if->uni_info;

  uni_id_len = pal_strlen (uni_info->uni_id);
  if (uni_id_len > ELMI_UNI_NAME_LEN)
    uni_id_len = ELMI_UNI_NAME_LEN;
  else if (uni_id_len == 0)
    {
      uni_id_len_null = PAL_TRUE;
      uni_id_len = ELMI_TLV_LEN;
    }

  /* Decrement len by 2 bytes (tlv type + tlv len )*/
  uni_len = (ELMI_UNI_FIXED_FRAME_LEN - 2)+ uni_id_len; 

  /* encode UNI Len */
  TLV_ENCODE_PUTC (uni_len);

  /* Encode CE-VLAN ID/EVC Map Type */
  TLV_ENCODE_PUTC (uni_info->cevlan_evc_map_type);

  /* Encode UNI Identifier sub IE */
  TLV_ENCODE_PUTC (ELMI_UNI_ID_SUB_IE_TLV);

  if (uni_id_len_null)
    { 
      /* Encode uni ID tlv length and value */
      TLV_ENCODE_PUTC (ELMI_TLV_LEN);
      TLV_ENCODE_PUTC (0x00);
    }
  else
    {
      TLV_ENCODE_PUTC (uni_id_len);
      TLV_ENCODE_PUT (uni_info->uni_id, uni_id_len);
    }

  /* Encode UNI BW profile parameters */
  elmi_encode_uni_bw_profile (pnt, size, port);

  return;

}

/* Encode cevlan/EVC MAP IE */
void
elmi_encode_cevlan_evc_map_tlv (u_char **pnt, u_int32_t *size,
                                struct elmi_port *port)
{
  u_int16_t cvlans_cnt = 0;
  u_int16_t temp_cvlans_cnt = 0;
  u_int16_t count = 0;
  u_int8_t multi_frames = PAL_FALSE;
  u_int16_t vid = 0; 
  u_int16_t cvlan_evc_len = 0; 
  u_int16_t tlv_len = 0; 
  u_int8_t tlv_val = 0; 
  u_int8_t last_ie = 0; 
  u_int8_t map_seq = 0; 
  u_int16_t evc_ref_id = 0; 
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;
  struct elmi_cvlan_evc_map *cevlan_evc_map = NULL;
  struct listnode *node = NULL;
  u_int16_t starting_svid = 0;
  u_int16_t last_evc_id = 0;
  int last_w = 0;
  int last_i = 0;
  int _w = 0;
  int _i = 0;
  u_int16_t last_cvid = 0;

  if (!port || !port->elmi_if)
    return;

  elmi_if = port->elmi_if;

  em = elmi_if->elmim;
  if (!em)
    return;

  if (!elmi_if->cevlan_evc_map_list)
    return;

  starting_svid = port->last_evc_id;

  zlog_info (ZG, "[UNIN-TX]: Encoding CEVLAN/EVC  MA Tlv");

  LIST_LOOP (elmi_if->cevlan_evc_map_list, cevlan_evc_map, node)
    {
      if (starting_svid != 0 && 
          port->last_ie_type == ELMI_LAST_CEVLAN_EVC_MAP_TLV)
        {
          if (cevlan_evc_map->evc_ref_id <= starting_svid)
            continue;
        }

      evc_ref_id = cevlan_evc_map->evc_ref_id;

      cvlans_cnt = 0;

      ELMI_VLAN_SET_BMP_ITER_BEGIN (cevlan_evc_map->cvlanbitmap, vid)
        {
          cvlans_cnt++;
        }
      ELMI_VLAN_SET_BMP_ITER_END (cevlan_evc_map->cvlanbitmap, vid);

      temp_cvlans_cnt = cvlans_cnt;

      /* Get maximum no. of octets required */
      cvlan_evc_len = ELMI_CEVLAN_EVC_MAP_FIXED_LEN + 
                      (ELMI_CVLAN_TLV_LEN * cvlans_cnt);

      if (ELMI_DEBUG(protocol, PROTOCOL))
        {
          zlog_err (ZG, "[UNIN-TX]: CEVLAN/EVC Map TLV len %d space left %d",
                    cvlan_evc_len, *size);
        }

      if (cvlan_evc_len > *size)
        {
          /* space not left to copy entire bytes */
          port->last_ie_type = ELMI_LAST_CEVLAN_EVC_MAP_TLV;
          port->last_evc_id = last_evc_id;

          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_info (ZG, "[UNIN-TX]: CEVLAN/EVC Map TLV Setting "
                       "last evc id = %d", last_evc_id); 
          return;
        }

      /* Get count of the vids. */
      cevlan_evc_map->cvlan_evc_map_seq = 0;

      if (cvlan_evc_len > ELMI_MAX_TLV)
        multi_frames = PAL_TRUE;

      if (multi_frames)
        {
          last_w = 0;
          last_i = 0;
          vid = 0;

          while (cvlans_cnt >= ELMI_MAX_CVLANS_IN_CMAP_TLV) 
            {
              /* encode cevlan map tlv */
              TLV_ENCODE_PUTC (ELMI_CEVLAN_EVC_MAP_IE_TLV);

              /* Decrement length for cevlan/evc map IE TLV type + TLV len */
              tlv_len = (ELMI_CEVLAN_EVC_MAP_FIXED_LEN - 2)+ 
                (ELMI_CEVLAN_EVC_MAP_MAX_VLAN_GRP * ELMI_CVLAN_TLV_LEN);

              TLV_ENCODE_PUTC (tlv_len);

              /* Encode Evc Ref Id */
              TLV_ENCODE_PUTW (evc_ref_id);

              last_ie = PAL_FALSE;

              /* check if it exceeds the maximum value */
              if (cevlan_evc_map->cvlan_evc_map_seq >= 
                  ELMI_CEVLAN_EVC_SEQ_NUM_MAX)
                cevlan_evc_map->cvlan_evc_map_seq = 0;

              map_seq = ++cevlan_evc_map->cvlan_evc_map_seq;

              tlv_val = (((last_ie << ELMI_SEQ_NUM_BITS) & 
                         ELMI_LAST_IE_BIT_MASK) 
                         || (map_seq & ELMI_SEQ_NUM_BIT_MASK));

              /* Encode last IE and Map seq number */
              TLV_ENCODE_PUTC (tlv_val);

              tlv_val = 0;             
              tlv_val = (cevlan_evc_map->untag_or_pri_frame << 1) | 
                cevlan_evc_map->default_evc; 

              /* Encode untagged priority/default evc */
              TLV_ENCODE_PUTC (tlv_val);

              count = 0; 

              TLV_ENCODE_PUTC (ELMI_EVC_MAP_ENTRY_SUB_IE_TLV);
          
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_info (ZG, "CEVLAN_EVC MAP entry tlv type = %X", 
                    ELMI_EVC_MAP_ENTRY_SUB_IE_TLV); 

              TLV_ENCODE_PUTC (temp_cvlans_cnt * ELMI_CVLAN_TLV_LEN);

              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_info (ZG, "CEVLAN_EVC MAP entry tlv len = %X", 
                    (temp_cvlans_cnt * ELMI_CVLAN_TLV_LEN)); 

              vid = 0; 

              for (_w = last_w; _w < ELMI_VLAN_BMP_WORD_MAX; _w++)                     
                for (_i = last_i; _i < ELMI_VLAN_BMP_WORD_WIDTH; _i++, (vid)++)
                  if ((cevlan_evc_map->cvlanbitmap).bitmap[_w] & 
                                                   (1U << _i))
                    {
                      TLV_ENCODE_PUTW (vid);

                      count++;
                      if (count >= ELMI_MAX_CVLANS_IN_CMAP_TLV)
                        { 
                          last_cvid = vid;
                          last_w = _w;
                          last_i = _i;
                          break;
                        }
                    }

              cvlans_cnt = cvlans_cnt - count;
            }
        }
      else
        {
          /* encode cevlan map tlv */
          TLV_ENCODE_PUTC (ELMI_CEVLAN_EVC_MAP_IE_TLV);

          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_info (ZG, "CEVLAN_EVC MAP TLV  = %d", 
                ELMI_CEVLAN_EVC_MAP_IE_TLV); 

          /* Decrement length for cevlan/evc map IE TLV type + tlv len */
          tlv_len = (ELMI_CEVLAN_EVC_MAP_FIXED_LEN - 2) + 
                    (cvlans_cnt * ELMI_CVLAN_TLV_LEN);

          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_info (ZG, "CEVLAN_EVC MAP Len = %d", tlv_len);

          TLV_ENCODE_PUTC (tlv_len);

          /* Encode Evc Ref Id */
          TLV_ENCODE_PUTW (evc_ref_id);

          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_info (ZG, "CEVLAN_EVC MAP evc_ref_id = %d", evc_ref_id);

          last_ie = PAL_TRUE;

          /* check if it exceeds the maximum value */
          if (cevlan_evc_map->cvlan_evc_map_seq >= ELMI_CEVLAN_EVC_SEQ_NUM_MAX)
            cevlan_evc_map->cvlan_evc_map_seq = 0;

          map_seq = ++cevlan_evc_map->cvlan_evc_map_seq;

          tlv_val = (((last_ie << ELMI_SEQ_NUM_BITS) & ELMI_LAST_IE_BIT_MASK)
                     | (map_seq & ELMI_SEQ_NUM_BIT_MASK));

          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_info (ZG, "CEVLAN_EVC MAP seq num tlv  = %X", tlv_val); 

          /* Encode last IE and Map seq number */
          TLV_ENCODE_PUTC (tlv_val);

          tlv_val = 0;             
          tlv_val = ((cevlan_evc_map->untag_or_pri_frame << 1) 
                     | cevlan_evc_map->default_evc); 

          TLV_ENCODE_PUTC (tlv_val);

          count = 0; 

          TLV_ENCODE_PUTC (ELMI_EVC_MAP_ENTRY_SUB_IE_TLV);
        
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_info (ZG, "CEVLAN_EVC MAP entry tlv type = %d", 
                ELMI_EVC_MAP_ENTRY_SUB_IE_TLV); 

          TLV_ENCODE_PUTC (temp_cvlans_cnt * ELMI_CVLAN_TLV_LEN);
 
          ELMI_VLAN_SET_BMP_ITER_BEGIN (cevlan_evc_map->cvlanbitmap, vid)
            {
              TLV_ENCODE_PUTW (vid);
            }
          ELMI_VLAN_SET_BMP_ITER_END (cevlan_evc_map->cvlanbitmap, vid);
        }

      last_evc_id = cevlan_evc_map->evc_ref_id;

    }

}

/* Encode EVC Status IE */
s_int32_t
elmi_encode_evc_status_tlv (u_char **pnt, u_int32_t *size,
                            struct elmi_port *port)
{
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;
  struct elmi_evc_status *evc_info = NULL;
  struct elmi_bridge *bridge = NULL;
  u_int8_t evc_status_ie_len = 0;
  struct listnode *node = NULL;
  u_int8_t evc_type = 0;
  u_int8_t evc_id_len = 0;
  u_int16_t starting_svid = 0;
  u_int16_t last_evc_id = 0;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  bridge = elmi_if->br;
  if (!bridge)
    return RESULT_ERROR;

  em  = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  if (!elmi_if->evc_status_list)
    return RESULT_ERROR;

  /* Check if the report type is full status continued msg and 
   * the last evc id is not zero
   */ 

  if (ELMI_DEBUG(protocol, PROTOCOL))
    {
      zlog_info(ZG, "[UNIN-TX]: Encoding EVC Status TLV");

      zlog_err (ZG, "[UNIN-TX]: EVCs last id = %d port->continued_msg = %d",
          port->last_evc_id, port->continued_msg);
    }

  if (port->last_evc_id != 0 && port->continued_msg)
    starting_svid = port->last_evc_id;

  LIST_LOOP (elmi_if->evc_status_list, evc_info, node)  
    {
      if (starting_svid != 0 && port->last_ie_type == ELMI_LAST_IE_EVC_TLV)
        {
          if (evc_info->evc_ref_id <= starting_svid)
            continue;
        }

      /* Check the size required to encode evc and ce-vlan map for that evc */
      evc_status_ie_len = 0;

      evc_status_ie_len = elmi_get_evc_length (port, evc_info);

      if (ELMI_DEBUG(protocol, PROTOCOL))
        {
          zlog_err (ZG, "[UNIN-TX]: EVC TLV Len = %d", evc_status_ie_len);
          zlog_err (ZG, "[UNIN-TX]: bytes left = %d", *size);
        }

      if (evc_status_ie_len > *size)
        {
          /* space not left to copy entire bytes */
          port->last_ie_type = ELMI_LAST_IE_EVC_TLV;
          port->last_evc_id  = last_evc_id;

          if (ELMI_DEBUG(protocol, PROTOCOL))
            {
              zlog_err (ZG, "[UNIN-TX]: No space left Sending EVC Tlv");
              zlog_err (ZG, "[UNIN-TX]: Last EVC id is set = %d", last_evc_id);
            }
          return ELMI_OK;
        }

      /* encode evc type tlv */
      TLV_ENCODE_PUTC (ELMI_EVC_STATUS_IE_TLV);

      /* Decrement evc tlv len by 2 bytes for 
       * evc status tlv type + evc status tlv len
       */

      /* Encode EVC Status IE Length */
      evc_status_ie_len -= ELMI_TLV_MIN_SIZE;

      TLV_ENCODE_PUTC (evc_status_ie_len);

      if (ELMI_DEBUG(protocol, PROTOCOL)) 
        zlog_err (ZG, "[UNIN-TX]: EVC Ref Id = %d\n", evc_info->evc_ref_id);

      /* encode EVC Ref Id */
      TLV_ENCODE_PUTW (evc_info->evc_ref_id);

      /* encode EVC status type */
      TLV_ENCODE_PUTC (evc_info->evc_status_type);

      /* encode EVC Parameters sub IE */
      TLV_ENCODE_PUTC (ELMI_EVC_PARAMS_SUB_IE_TLV);

      /* encode EVC parameters length tlv */
      TLV_ENCODE_PUTC (ELMI_EVC_PARAMS_SUB_IE_TLV_LEN);

      evc_type = evc_info->evc_type & ELMI_EVC_TYPE_MASK;

      /* encode EVC parameters value */
      TLV_ENCODE_PUTC (evc_type);

      /* Encode EVC ID Sub IE */
      TLV_ENCODE_PUTC (ELMI_EVC_ID_SUB_IE_TLV);

      evc_id_len = pal_strlen (evc_info->evc_id);
      if (!evc_id_len)
        {
          TLV_ENCODE_PUTC (ELMI_TLV_LEN);
          TLV_ENCODE_PUTC (0x00);
        }
      else if (evc_id_len > ELMI_EVC_NAME_LEN)
        {
          TLV_ENCODE_PUTC (ELMI_EVC_NAME_LEN);
          TLV_ENCODE_PUT (evc_info->evc_id, ELMI_EVC_NAME_LEN);
        }
      else
        {
          TLV_ENCODE_PUTC (evc_id_len);
          TLV_ENCODE_PUT (evc_info->evc_id, evc_id_len);
        }

      /* Encode Bandwidth Profile Parameters */
      elmi_encode_evc_bw_profile (pnt, size, port, evc_info);

      last_evc_id = evc_info->evc_ref_id; 
    }

  return RESULT_OK;

}

/*******************************************************
  Functions for frame send for UNI_N TX module
 ********************************************************/

/* Send Statue Enquiry Message */
s_int32_t
elmi_unin_send_echeck_msg (u_char **pnt,
                           u_int32_t *size, 
                           struct elmi_port *port)
{

  /* Encode common header */
  elmi_unin_encode_header (pnt, size, ELMI_REPORT_TYPE_ECHECK);

  /* Encode seq num and data instance tlvs */
  elmi_unin_encode_seqnum_and_di (pnt, size, port);

  /* initialize last evc_id to zero */
  port->last_evc_id = 0;

  return RESULT_OK;
}

/* This function calculates total bytes to be sent 
 * mainly to decide to send full status or full status continued message 
 */ 
s_int32_t
elmi_unin_compute_total_len (struct elmi_port *port)
{
  u_int16_t total_bytes = 0;
  u_int16_t evc_len = 0;
  u_int32_t uni_len = 0;
  u_int32_t uni_id_len = 0;
  u_int32_t map_len = 0;
  struct elmi_uni_info *uni_info = NULL;
  struct elmi_evc_status *evc_info = NULL;
  struct elmi_cvlan_evc_map *cvlan_map_info = NULL;
  struct listnode *node = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em = elmi_if->elmim;
  /*
   * Get the length of all the components.
   * Check if the length exceeds the MAX_MTU_SIZE, 
   */

  total_bytes = ELMI_HEADER_FIXED_FRAME_LEN; 

  uni_info = port->elmi_if->uni_info;
  if (uni_info)
    {
      uni_id_len = pal_strlen(uni_info->uni_id); 
      if (uni_id_len > ELMI_UNI_NAME_LEN)
        uni_id_len = ELMI_UNI_NAME_LEN;
      else if (uni_id_len == 0)
        uni_id_len = ELMI_TLV_LEN;

      uni_len = ELMI_UNI_FIXED_FRAME_LEN + uni_id_len;

      total_bytes += uni_len;
    }

  LIST_LOOP (elmi_if->evc_status_list, evc_info, node)
    {
      evc_len = elmi_get_evc_length (port, evc_info);
      total_bytes += evc_len;
    }

  LIST_LOOP(elmi_if->cevlan_evc_map_list, cvlan_map_info, node)
    {
      map_len = elmi_cvlan_map_len (cvlan_map_info);
      total_bytes += map_len;
    }
  
  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_info (ZG, "[UNIN-TX]: Total computed Len = %d", total_bytes);

  return total_bytes;

}

/* Send Full status Message */
s_int32_t
elmi_unin_send_fullstatus_msg (u_char **pnt,
                               u_int32_t *size, 
                               struct elmi_port *port)
{
  s_int32_t ret = 0;
  struct elmi_master *em = NULL;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  em = port->elmi_if->elmim; 

  elmi_unin_encode_header (pnt, size, ELMI_REPORT_TYPE_FULL_STATUS);

  /* encode seq num and data instance tlvs */
  elmi_unin_encode_seqnum_and_di (pnt, size, port);

  /* Uni status need not be encoded if this status msg 
   * is continuation of previous full status contd msg
   */  

  /* Encode UNI Status */
  if (!port->continued_msg)
    elmi_encode_uni_status_tlv (pnt, size, port);

  if ((!port->continued_msg) ||
   ((port->continued_msg) && (port->last_ie_type == ELMI_LAST_IE_EVC_TLV)))
  /* Encode EVC Status */
  ret = elmi_encode_evc_status_tlv (pnt, size, port); 

  if (ret < 0)
   {
     if (ELMI_DEBUG(protocol, PROTOCOL))
       zlog_info (ZG, "[UNIN-TX]: Error in EVC encoding");
     return ret;
   }

  elmi_encode_cevlan_evc_map_tlv (pnt, size, port);

  port->last_evc_id = 0;
  port->last_ie_type = 0;
  port->continued_msg = PAL_FALSE;

  return ret;
}

s_int32_t
elmi_unin_send_fullstatus_contd_msg (u_char **pnt,
                                     u_int32_t *size, 
                                     struct elmi_port *port)
{
  s_int32_t ret = 0;
  struct elmi_master *em = NULL;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  em = port->elmi_if->elmim;

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_info (ZG, "Sending Full status continued message");

 /* Encode common fields */
  elmi_unin_encode_header (pnt, size, ELMI_REPORT_TYPE_FULL_STATUS_CONTD);

  /* encode seq num and data instance tlvs */
  elmi_unin_encode_seqnum_and_di (pnt, size, port);

  /* ENCODE UNI Status 
   * In full status contd pdu encode uni status only in the first frame
   */
  if (!port->last_evc_id)
    elmi_encode_uni_status_tlv (pnt, size, port);

  /* ENCODE EVC Status */
  if ((!port->continued_msg) ||
      ((port->continued_msg) && (port->last_ie_type == ELMI_LAST_IE_EVC_TLV)))
    ret = elmi_encode_evc_status_tlv (pnt, size, port);

  /* If there is no space left */
  if (ret != ELMI_OK)
    elmi_encode_cevlan_evc_map_tlv (pnt, size, port);

  return RESULT_OK;
}

/* To send async single EVC MSGs */
s_int32_t
elmi_unin_send_async_evc_msg (struct elmi_evc_status *evc_info, 
                              struct elmi_port *port)
{
  u_int8_t buf[ELMI_MIN_PACKET_SIZE];
  u_int8_t **pnt;
  u_int8_t *bufptr = NULL;
  u_int32_t *size = NULL;
  u_int32_t buf_len = 0;
  u_int32_t length = 0;
  u_int32_t padding_len = 0;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;
  struct elmi_evc_status *evc_instance = NULL;
  struct elmi_cvlan_evc_map *cevlan_evc = NULL;
  s_int32_t ret = 0;

  bufptr = buf;
  pnt = &bufptr;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  ifp = port->elmi_if->ifp;
  if (!ifp)
    return RESULT_ERROR;

  /* Do not send the frame, if the interface is down */
  if (!if_is_up (ifp))
    return RESULT_ERROR;

  em = port->elmi_if->elmim;
  if (em == NULL)
    return RESULT_ERROR;

  if (bufptr == NULL)
    return RESULT_ERROR;

  if (!evc_info)
    return RESULT_ERROR;

  buf_len = ELMI_MIN_PACKET_SIZE;
  size = &buf_len;

  elmi_unin_encode_header (pnt, size, ELMI_REPORT_TYPE_SINGLE_EVC);

  /* Seq number and DI tlvs won't present for traps */

  /* encode evc type tlv */
  TLV_ENCODE_PUTC (ELMI_EVC_STATUS_IE_TLV);

  /* Encode EVC Status IE Length */
  TLV_ENCODE_PUTC (ELMI_EVC_ASYNC_EVC_LEN);

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_info (ZG, "ASYNC PDU EVC Ref Id = %d\n", evc_info->evc_ref_id);

  /* encode EVC Ref Id */
  TLV_ENCODE_PUTW (evc_info->evc_ref_id);

  /* Only evc status will be present in the async evc msg */
  TLV_ENCODE_PUTC (evc_info->evc_status_type);

  /* for UNI_C length will be always equal to ELMI_MIN_PACKET_SIZE */
  length = ELMI_MIN_PACKET_SIZE - (*size);
  if (length < ELMI_MIN_PACKET_SIZE)
    {
      padding_len = ELMI_MIN_PACKET_SIZE - length;
      TLV_ENCODE_PUT_EMPTY (padding_len);
      length += padding_len;
    }

  /* Send out frame */
  ret = elmi_frame_send (elmi_if->dev_addr, elmi_dest_addr,
                         elmi_if->ifindex, buf, length);

  /* if the event is delete evc, then delete the evcs */
  if (evc_info->active == PAL_FALSE)
    {
      /* Get ce-vlan evc map info */
      cevlan_evc = elmi_lookup_cevlan_evc_map (elmi_if,
          evc_info->evc_ref_id);

      if (cevlan_evc)
        {
          listnode_delete (elmi_if->cevlan_evc_map_list,
              (void *)cevlan_evc);
          XFREE (MTYPE_ELMI_CEVLAN_EVC, cevlan_evc);
        }

      /* Delete this evc from EVCs list */
      evc_instance = elmi_evc_look_up (elmi_if, evc_info->evc_ref_id);
      if (evc_instance)
        {
          listnode_delete (elmi_if->evc_status_list, evc_instance);
          XFREE (MTYPE_ELMI_EVC, evc_instance);
        }

    }

  return ret;
}

/* Transmit response to UNI-C */
s_int32_t
elmi_uni_n_tx (struct elmi_port *port, u_int8_t report_type)
{
  u_char buf[ELMI_MAX_PACKET_SIZE];
  u_char **pnt;
  u_char *bufptr = NULL;
  u_int32_t *size = NULL;
  u_int32_t padding_len = 0;
  u_int32_t buf_len = 0;
  u_int32_t length = 0;
  struct interface *ifp = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct elmi_master *em = NULL;
  s_int32_t ret = 0;

  bufptr = buf;
  pnt = &bufptr;

  if (!port || !port->elmi_if)
    return RESULT_ERROR;

  ifp = port->elmi_if->ifp;
  if (!ifp)
    return RESULT_ERROR;

  /* Do not send the frame, if the interface is down */
  if (!if_is_up (ifp)) 
    return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em = elmi_if->elmim;
  if (em == NULL)
    return RESULT_ERROR;

  if (bufptr == NULL)
    return RESULT_ERROR;

  buf_len = ELMI_MAX_PACKET_SIZE;
  size = &buf_len;

    switch (report_type)
    {
    case ELMI_REPORT_TYPE_FULL_STATUS:
      ret = elmi_unin_send_fullstatus_msg (pnt, size, port); 
      break;
    case ELMI_REPORT_TYPE_ECHECK:
      ret = elmi_unin_send_echeck_msg (pnt, size, port); 
      break;
    case ELMI_REPORT_TYPE_FULL_STATUS_CONTD:
      ret = elmi_unin_send_fullstatus_contd_msg (pnt, size, port);
      break;
    default:
      if (ELMI_DEBUG(packet, TX))
        zlog_err (ZG, "[UNIN-TX]:Invalid Report Type");
      break;
    }

  if (ret < 0)
    return RESULT_ERROR;

  length = ELMI_MAX_PACKET_SIZE - (*size);
  if (length < ELMI_MIN_PACKET_SIZE)
    {
      padding_len = ELMI_MIN_PACKET_SIZE - length;
      TLV_ENCODE_PUT_EMPTY (padding_len);
      length += padding_len;
    }
  
  /* Send out frame */
  ret = elmi_frame_send (elmi_if->dev_addr, elmi_dest_addr, 
                         elmi_if->ifindex, buf, length);
  
  if (ELMI_DEBUG(packet, TX))
    elmi_hexdump (buf, length);

  if (ret < 0)
    return ret;

  /* Restart PVT if it is enabled */
  if (elmi_if->enable_pvt == PAL_TRUE)
    {
      /* operational state will be changed only when PVT is enabled */

      /* Received request from UNI-C */
      port->rcnt_success_oper_cnt++;

      /* Reset status counter */
      port->current_status_counter = ELMI_COUNTER_MIN;

      /* If the recent N393 operations are success, ELMI becomes Operational */
      if (port->rcnt_success_oper_cnt == elmi_if->status_counter_limit)
        {
          /* Change ELMI operational state
           * Send Update to NSM if there is a ELMI Operational state change
           */
          if (port->elmi_operational_state != ELMI_OPERATIONAL_STATE_UP)
            {
              port->elmi_operational_state = ELMI_OPERATIONAL_STATE_UP;

              if (ELMI_DEBUG (protocol, PROTOCOL))
                zlog_debug (ZG, "PVT: "
                    "Response rcvd for the last N393, state transition: UP");

              /* Send notification to NSM */
              elmi_nsm_send_operational_status (port,
                                                port->elmi_operational_state);
            }
        }

      if (l2_is_timer_running (port->polling_verification_timer))
        l2_stop_timer (&port->polling_verification_timer);

      port->polling_verification_timer =
        l2_start_timer (elmi_polling_verification_timer_handler,
                        (void *)port,
                        SECS_TO_TICS(elmi_if->polling_verification_time), ZG);
    }

  /* Store the sent report type */
  port->last_request_type = report_type;

  port->rcvd_response = PAL_FALSE;

  return ret;
}

/*************************************/
/* Packet RX Part                    */
/*************************************/
s_int32_t
elmi_unin_validate_rcvd_pdu (struct elmi_ifp *elmi_if, u_char **pnt,
                             s_int32_t *size,
                             struct elmi_pdu *pdu)

{
  struct elmi_port *port  = NULL;
  struct elmi_master *em  = NULL;
  u_int8_t report_type_tlv = 0;
  u_int8_t report_type_len = 0;
  u_int8_t tlv_type = 0;
  u_int8_t tlv_length = 0;
  u_int8_t tlv_val = 0;
  u_int8_t rcvd_sent_seq_counter = 0;

  if (!elmi_if)
    return RESULT_ERROR;

  em = elmi_if->elmim;

  if (!em)
    return RESULT_ERROR;

  port = elmi_if->port;

  /* Parse protocol version */
  TLV_DECODE_GETC (pdu->proto_ver);

  if (pdu->proto_ver != ELMI_PROTO_VERSION)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIN-RX]: Received invalid protocol version");

      port->stats.inval_protocol_ver++;
      return RESULT_ERROR;
    }

  /* Parse Message Type */
  TLV_DECODE_GETC (pdu->msg_type);

  if (pdu->msg_type != ELMI_MSG_TYPE_STATUS_ENQUIRY)
    {
      /* Check if the protocol version repeated */
      if (pdu->msg_type == ELMI_PROTO_VERSION)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIN-RX]: Received Duplicate protocol version");

          /* Ignore the duplicate element */
          port->stats.duplicate_ie++;

          /* Parse Message Type */
          TLV_DECODE_GETC (pdu->msg_type);
          if (pdu->msg_type != ELMI_MSG_TYPE_STATUS_ENQUIRY)
            {
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIN-RX]: Received invalid Message Type");

              port->stats.inval_msg_type++;
              return RESULT_ERROR;
            }
        }
      else
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIN-RX]: Received Invalid Message Type");

          port->stats.inval_msg_type++;
          return RESULT_ERROR;
        }

    }

  /* Decode the report type field */
  TLV_DECODE_GETC (report_type_tlv);

  if (report_type_tlv != ELMI_REPORT_TYPE_IE_TLV)
    {
      /* Check if the message type repeated */
      if (report_type_tlv == ELMI_MSG_TYPE_STATUS_ENQUIRY)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIN-RX]: Received Duplicate message type");

          /* Ignore the duplicate element */
          port->stats.duplicate_ie++;

          /* Parse Report Type */
          TLV_DECODE_GETC (report_type_tlv);

          if (report_type_tlv != ELMI_REPORT_TYPE_IE_TLV) 
            {
              if (ELMI_DEBUG(protocol, PROTOCOL))
                zlog_err (ZG, "[UNIC-RX]: Received invalid  report type");

              port->stats.mandatory_ie_missing++;
              return RESULT_ERROR;
            }
        }
      else
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIN-RX]: Received Invalid Report Type\n");

          port->stats.mandatory_ie_missing++;
          return RESULT_ERROR;
        }
    }

  /* Parse report type length */
  TLV_DECODE_GETC (report_type_len);

  if (report_type_len != ELMI_REPORT_TYPE_IE_LENGTH)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIN-RX]: Received Invalid Report length");

      port->stats.inval_mandatory_ie++;
      return RESULT_ERROR;
    }

  /* Parse report type val */
  TLV_DECODE_GETC (pdu->report_type);

  if (pdu->report_type == ELMI_INVALID_REPORT_TYPE)
    {
      /* Requested for full status, but received keep alive msg */
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Invalid Report Type");
      
       port->stats.inval_mandatory_ie++;

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

          if (ELMI_DEBUG(packet, RX))
            zlog_err (ZG, "[UNIN-RX]: Received duplicate TLV ignore");

            port->stats.duplicate_ie++;

          /* skip the val by length */
          TLV_DECODE_SKIP (tlv_length);
          TLV_DECODE_GETC (tlv_val);

          /* next tlv should be seq number tlv */
          if (tlv_type != ELMI_SEQUENCE_NUM_IE_TLV)
            {
              port->stats.mandatory_ie_missing++;
              return RESULT_ERROR;
            }
        }
      else
        {
          if (ELMI_DEBUG(packet, RX))
            zlog_err (ZG, "[UNIN-RX]: Received Invalid sequence number TLV");

          port->stats.mandatory_ie_missing++;
          return RESULT_ERROR;
        }
    }

  /* Parse sequence number length */
  TLV_DECODE_GETC (tlv_length);

  if (tlv_length != ELMI_SEQUENCE_NUM_IE_LENGTH )
    {
      if (ELMI_DEBUG(packet, RX))
        zlog_err (ZG, "[UNIN-RX]: Received sequence number tlv with "
                  "invalid TLV");

      port->stats.inval_mandatory_ie++;
      return RESULT_ERROR;
    }

  /* Parse sent sequence number val */
  TLV_DECODE_GETC (pdu->rcvd_seq_num);

  if (!pdu->rcvd_seq_num)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIC-RX]: Received seq num val is zero invalid");

      port->stats.inval_seq_num++;
      return RESULT_ERROR;
    }

  /* Parse received receive sequence number val */
  TLV_DECODE_GETC (rcvd_sent_seq_counter);

  /* Check for process restart state */
  if (rcvd_sent_seq_counter == 0)
    {
      if (pdu->rcvd_seq_num == ELMI_SEQ_NUM_MIN)
        {
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIN-RX]: Indicates UNI-C just started");
        }
      else
        port->stats.inval_seq_num++;
    }
  else if (rcvd_sent_seq_counter != port->sent_seq_num)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIN-RX]: sequence number val mismatch");
      
       port->stats.inval_seq_num++;
      /* UNIN will ignore the seq num mismatch error and 
       * send requested info
       */  
    }

  /* Decode data instance field */
  TLV_DECODE_GETC (tlv_type);

  if (tlv_type != ELMI_DATA_INSTANCE_IE_TLV)
    {
      if (tlv_type == ELMI_SEQUENCE_NUM_IE_TLV)
        {
          /* Check if any IE is repeated, ignore it */
          TLV_DECODE_GETC (tlv_length);

          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_err (ZG, "[UNIN-RX]: Received duplicate TLV ignore");

          port->stats.duplicate_ie++;

          /* skip the val by length */
          TLV_DECODE_SKIP (tlv_length);
          TLV_DECODE_GETC (tlv_type);

          if (tlv_type != ELMI_DATA_INSTANCE_IE_TLV)
            {
              port->stats.mandatory_ie_missing++;
              return RESULT_ERROR;
            }
        }
      else
        {
          /* check if any tlv is repeated */
          if (ELMI_DEBUG(protocol, PROTOCOL))
            zlog_debug (ZG, "[UNIN-RX]: Received Wrong"
                        " data instance field on port %d", elmi_if->ifindex);

          port->stats.mandatory_ie_missing++;
          return RESULT_ERROR;
        }
    }

  TLV_DECODE_GETC (tlv_length);

  if (tlv_length != ELMI_DATA_INSTANCE_IE_LENGTH)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIN-RX]: Received invalid data instance val");

      port->stats.inval_mandatory_ie++;
      return RESULT_ERROR;
    }

  TLV_DECODE_SKIP (ELMI_TLV_LEN);
  TLV_DECODE_GETL (tlv_val);

  /* Data instance value */
  if (!tlv_val)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIN-RX]: UNI-C data instance value is zero");
    }

  /* verify if any other unexpected tlvs present in the msg */
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

  return RESULT_OK;

}

/* Parse ELMI status enquiry message (Keep alives) */
s_int32_t
elmi_unin_process_elmi_check_msg (struct elmi_port *port,
                                  struct elmi_pdu *pdu) 
{
  s_int32_t ret = 0;
  struct elmi_master *em = NULL;

  if (!port)
   return RESULT_ERROR;

  em = port->elmi_if->elmim;
  if (!em)
    return RESULT_ERROR; 

  port->recvd_send_seq_num = pdu->rcvd_seq_num;
  port->rcvd_response = PAL_TRUE;

  /* Reply to ELMI status enquiry message */
  elmi_uni_n_tx (port, pdu->report_type);

  return ret;

}

/* Parse FULL STATUS message */
s_int32_t
elmi_unin_process_fullstatus_msg (struct elmi_port *port, 
                                  struct elmi_pdu *pdu) 
{
  struct elmi_master *em = NULL;
  struct elmi_evc_status *evc_trap_info = NULL;
  struct listnode *trap_node = NULL;
  struct listnode *next_node = NULL;
  s_int32_t total_len = 0;

  if (!port)
   return RESULT_ERROR;

  em = port->elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  /* Save received sent sequnce number */
  port->recvd_send_seq_num = pdu->rcvd_seq_num;
  port->rcvd_response = PAL_TRUE;

  /* At UNI-N we do not store Data instance */ 

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_err (ZG, "[UNIN-RX]: Received Request for FULL status msg");

  /* Check if any evc notifications are pending in the queue. 
   * If yes, then delete the pending notifications from the queue 
   * and reset the asynchronous evc timer 
   * */

  if (port->async_evc_list != NULL)
    {
      /* stop the timer */
      if (l2_is_timer_running(port->async_timer))
        {
          l2_stop_timer (&port->async_timer);
        }

      /* delete async traps list */
      if (port->async_evc_list)
        {
          LIST_LOOP_DEL (port->async_evc_list, evc_trap_info, trap_node, 
                         next_node)
            listnode_delete (port->async_evc_list, evc_trap_info);
        }
    }

  port->last_evc_id = 0;
  port->last_ie_type = 0;
  port->continued_msg = PAL_FALSE;

  /* Check report type to be send : FULL STATUS/FULL STATUS OCNTD */
  total_len = elmi_unin_compute_total_len (port);

  if (total_len > ELMI_MAX_PACKET_SIZE)
    elmi_uni_n_tx (port, ELMI_REPORT_TYPE_FULL_STATUS_CONTD); 
  else
    elmi_uni_n_tx (port, ELMI_REPORT_TYPE_FULL_STATUS);

  return RESULT_OK;
}

/* Parse FULL STATUS Continued Message */
s_int32_t
elmi_unin_process_fullstatus_contd_msg (struct elmi_port *port,
                                        struct elmi_pdu *pdu)
{
  struct elmi_master *em = NULL;
  struct elmi_evc_status *evc_info = NULL;
  struct elmi_cvlan_evc_map *cevlan_evc_map = NULL;
  struct listnode *node = NULL;
  struct elmi_ifp *elmi_if = NULL;
  u_int16_t cvlan_evc_len = 0;
  u_int16_t evc_len = 0;
  u_int16_t total_len = 0;

  if (!port || !port->elmi_if)
   return RESULT_ERROR;

  elmi_if = port->elmi_if;

  em = port->elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_info (ZG, "Received request for FULL Status Continued pdu");

  /* Save received sent sequnce number */
  port->recvd_send_seq_num = pdu->rcvd_seq_num;
  port->rcvd_response = PAL_TRUE;
  port->continued_msg = PAL_TRUE;

  /* Calculate to send full status or full status contd pdu */
  LIST_LOOP (elmi_if->evc_status_list, evc_info, node)
    {
      if (port->last_ie_type == ELMI_LAST_IE_EVC_TLV)
        {
          if (evc_info->evc_ref_id <= port->last_evc_id)
            continue;

          evc_len += elmi_get_evc_length (port, evc_info);
        }
      else
        continue;
    }

  LIST_LOOP (elmi_if->cevlan_evc_map_list, cevlan_evc_map, node)
    {
      if (port->last_ie_type == ELMI_LAST_IE_EVC_TLV)
        {
          cvlan_evc_len += elmi_cvlan_map_len (cevlan_evc_map);
        }
      else
        {
          if (cevlan_evc_map->evc_ref_id <= port->last_evc_id)
            continue;

          cvlan_evc_len += elmi_cvlan_map_len (cevlan_evc_map);

        }
    }

  total_len = evc_len + cvlan_evc_len;

  if (ELMI_DEBUG(protocol, PROTOCOL))
    zlog_info (ZG, "[UNIN-TX]: CONTD PDU: Remaining bytes to be sent %d", 
               total_len);

  /* Fixed frame len includes common header len */
  if (total_len > (ELMI_MAX_PACKET_SIZE - ELMI_HEADER_FIXED_FRAME_LEN))
    {
      /* Total bytes to be send exceeds the max MTU size  */
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_info (ZG, "[UNIN-TX]: Sending next Full Status continued pdu");

      elmi_uni_n_tx (port, ELMI_REPORT_TYPE_FULL_STATUS_CONTD);
    }
  else
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_info (ZG, "[UNIN-TX]: Sending next Full Status pdu");

      elmi_uni_n_tx (port, ELMI_REPORT_TYPE_FULL_STATUS);
    }

  return RESULT_OK;

}

/* Process the Received frames */
s_int32_t
elmi_unin_handle_frame (struct lib_globals *zg, 
                        struct sockaddr_l2 *l2_skaddr,
                        u_char **pnt, s_int32_t *size)

{
  struct apn_vr *vr = apn_vr_get_privileged (zg);
  struct elmi_port *port = NULL;
  struct elmi_ifp *elmi_if = NULL;
  struct interface *ifp = NULL;
  struct elmi_bridge *br = NULL;
  struct elmi_master *em = NULL;
  struct elmi_pdu pdu;
  s_int32_t ret = 0;

  /* Get port info */
  if (!l2_skaddr || !l2_skaddr->port)
    return RESULT_ERROR;

  ifp = if_lookup_by_index (&vr->ifm, l2_skaddr->port);

  elmi_if = ifp->info;

  if (!elmi_if || !elmi_if->port)
    return RESULT_ERROR;

  em = elmi_if->elmim;
  if (!em)
    return RESULT_ERROR;
  port = elmi_if->port;

  br = port->elmi_if->br;

  if (!br || !elmi_if->elmi_enabled)
    return RESULT_ERROR;

  if (!elmi_if->elmi_enabled)
    {
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_debug (ZG, "[UNIN-RX]: ELMI not enabled on port %d", 
                    ifp->ifindex);

      return RESULT_ERROR;
    }

  pal_mem_set (&pdu, 0, sizeof(struct elmi_pdu));

  ret = elmi_unin_validate_rcvd_pdu (elmi_if, pnt, size, &pdu);
  if (ret < 0)
    return RESULT_ERROR;

  switch (pdu.report_type)
    {
    case ELMI_REPORT_TYPE_FULL_STATUS:
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIN-RX]: Received Full status Request");
     
      port->stats.last_full_status_enq_rcvd = pal_time_current (0);

      ret = elmi_unin_process_fullstatus_msg (port, &pdu);
      break;

    case ELMI_REPORT_TYPE_ECHECK:
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIN-RX]: Received status enquiry Request");

      ret = elmi_unin_process_elmi_check_msg (port, &pdu);
      break;

    case ELMI_REPORT_TYPE_FULL_STATUS_CONTD:
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIN-RX]: Received full status continued Request");

      ret = elmi_unin_process_fullstatus_contd_msg (port, &pdu);
      break;

    default:
      if (ELMI_DEBUG(protocol, PROTOCOL))
        zlog_err (ZG, "[UNIN-RX]: Received Invalid Report Type");
      break;
    }

  if (ret < 0)
    return RESULT_ERROR;

  return RESULT_OK;
}
