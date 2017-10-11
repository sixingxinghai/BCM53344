/* Copyright (C) 2004   All Rights Reserved. */

#include "pal.h"
#include "lib.h"
#include "tlv.h"

#include "hal_incl.h"
#include "hal_debug.h"
#include "hal_msg.h"
#include "hal_netlink.h"
#include "hal_comm.h"
#include "hal_nsm.h"

/* 
   Header encode.  
*/
int
hal_msg_encode_tlv (u_char **pnt, u_int32_t *size, u_int16_t type,
                    u_int16_t length)
{
  u_char *sp = *pnt;

  if (*size < HAL_MSG_TLV_HEADER_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  length += HAL_MSG_TLV_HEADER_SIZE;

  TLV_ENCODE_PUTW (type);
  TLV_ENCODE_PUTW (length);

  return *pnt - sp;
}

/* 
   Encode interface message.
*/
int
hal_msg_encode_if (u_char **pnt, u_int32_t *size, struct hal_msg_if *msg)
{
  unsigned char *sp = *pnt;
  u_int16_t i, j;

  /* Check size. */
  if (*size < HAL_MSG_IF_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Interface name. */
  TLV_ENCODE_PUT (msg->name, HAL_IFNAME_LEN + 1);
  
  /* Interface index. */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Interface type. L2/L3 */
  TLV_ENCODE_PUTC (msg->type);
  
  /* Interface TLVs. */
  for (i = 0; i < CINDEX_SIZE; i++)
    if (CHECK_CINDEX (msg->cindex, i))
      {
        if (*size < HAL_MSG_TLV_HEADER_SIZE)
          return HAL_MSG_PKT_TOO_SMALL;

        switch (i)
          {
          case HAL_MSG_CINDEX_IF_FLAGS:
            hal_msg_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->flags);
            break;
          case HAL_MSG_CINDEX_IF_METRIC:
            hal_msg_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->metric);
            break;
          case HAL_MSG_CINDEX_IF_MTU:
            hal_msg_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTW (msg->mtu);
            break;
          case HAL_MSG_CINDEX_IF_DUPLEX:
            hal_msg_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTW (msg->duplex);
            break;
          case HAL_MSG_CINDEX_IF_ARP_AGEING_TIMEOUT:
            hal_msg_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTW (msg->arp_ageing_timeout);
            break;
          case HAL_MSG_CINDEX_IF_AUTONEGO:
            hal_msg_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->autonego);
            break;
          case HAL_MSG_CINDEX_IF_HW:
            hal_msg_encode_tlv (pnt, size, i, HAL_HW_LENGTH + 2 + 2);
            TLV_ENCODE_PUTW (msg->hw_type);
            TLV_ENCODE_PUTW (msg->hw_addr_len);
            TLV_ENCODE_PUT (msg->hw_addr, HAL_HW_LENGTH);
            break;
          case HAL_MSG_CINDEX_IF_BANDWIDTH:
            hal_msg_encode_tlv (pnt, size, i, 4);
            TLV_ENCODE_PUTL (msg->bandwidth);
            break;
          case HAL_MSG_CINDEX_IF_SEC_HW_ADDRESSES:
            hal_msg_encode_tlv (pnt, size, i, 2 + msg->nAddrs * msg->sec_hw_addr_len);
            TLV_ENCODE_PUTW (msg->sec_hw_addr_len);

            for (j = 0; j < msg->nAddrs; j++)
              {
                TLV_ENCODE_PUT (msg->addresses[j], msg->sec_hw_addr_len);
              }
            break;
          default:
            break;
          }
      }

  return *pnt - sp;
}

/* 
   Decode interface message. 
*/
int
hal_msg_decode_if (u_char **pnt, u_int32_t *size, struct hal_msg_if *msg)
{
  struct hal_msg_tlv_header tlv;
  unsigned char *sp = *pnt;
  int i;

  pal_mem_set (msg, 0, sizeof (struct hal_msg_if));

  /* Check size. */
  if (*size < HAL_MSG_IF_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Interface name. */
  TLV_DECODE_GET (msg->name, HAL_IFNAME_LEN + 1);

  /* Interface index. */
  TLV_DECODE_GETL (msg->ifindex);
  
  /* Interface type. L2/L3 */
  TLV_DECODE_GETC (msg->type);

  /* TLV parse. */
  while (*size)
    {
      if (*size < HAL_MSG_TLV_HEADER_SIZE)
        return HAL_MSG_PKT_TOO_SMALL;

      HAL_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case HAL_MSG_CINDEX_IF_FLAGS:
          TLV_DECODE_GETL (msg->flags);
          SET_CINDEX (msg->cindex, tlv.type);
          break;
        case HAL_MSG_CINDEX_IF_METRIC:
          TLV_DECODE_GETL (msg->metric);
          SET_CINDEX (msg->cindex, tlv.type);
          break;
        case HAL_MSG_CINDEX_IF_MTU:
          TLV_DECODE_GETW (msg->mtu);
          SET_CINDEX (msg->cindex, tlv.type);
          break;
        case HAL_MSG_CINDEX_IF_DUPLEX:
          TLV_DECODE_GETW (msg->duplex);
          SET_CINDEX (msg->cindex, tlv.type);
          break;
        case HAL_MSG_CINDEX_IF_ARP_AGEING_TIMEOUT:
          TLV_DECODE_GETW (msg->arp_ageing_timeout);
          SET_CINDEX (msg->cindex, tlv.type);
          break;
        case HAL_MSG_CINDEX_IF_AUTONEGO:
          TLV_DECODE_GETL (msg->autonego);
          SET_CINDEX (msg->cindex, tlv.type);
          break;
        case HAL_MSG_CINDEX_IF_HW:
          TLV_DECODE_GETW (msg->hw_type);
          TLV_DECODE_GETW (msg->hw_addr_len);
          TLV_DECODE_GET (msg->hw_addr, HAL_HW_LENGTH);
          SET_CINDEX (msg->cindex, tlv.type);
          break;
        case HAL_MSG_CINDEX_IF_BANDWIDTH:
          TLV_DECODE_GETL (msg->bandwidth);
          SET_CINDEX (msg->cindex, tlv.type);
          break;
        case HAL_MSG_CINDEX_IF_SEC_HW_ADDRESSES:
          TLV_DECODE_GETW (msg->sec_hw_addr_len);
          msg->nAddrs = tlv.length / msg->sec_hw_addr_len;
          msg->addresses = XCALLOC (MTYPE_TMP, sizeof (u_int8_t *));
          if (msg->addresses == NULL)
            goto CLEANUP;
          for (i = 0; i < msg->nAddrs; i++)
            {
              msg->addresses[i] = XCALLOC (MTYPE_TMP, msg->sec_hw_addr_len);
              if (msg->addresses[i] == NULL)
                goto CLEANUP;
              TLV_DECODE_GET (msg->addresses[i], msg->sec_hw_addr_len);
            }
          SET_CINDEX (msg->cindex, tlv.type);
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;
        }
    }

  return *pnt - sp;

 CLEANUP:

  if (msg->addresses != NULL)
    {
      for (i = 0; i < msg->nAddrs; i++)
        if (msg->addresses[i] != NULL)
          XFREE (MTYPE_TMP, msg->addresses[i]);
      XFREE (MTYPE_TMP, msg->addresses);
    }

  return -1;
}



#ifdef HAVE_LACPD
/* 
   Encode lacp set port selection criteria.
*/
int
hal_msg_encode_lacp_psc_set (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_psc_set *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_LACP_PSC_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Interface index. */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* PSC type. */
  TLV_ENCODE_PUTL (msg->psc);
  
  return *pnt - sp;
}

/* 
   Decode lacp set port selection criteria. 
*/
int
hal_msg_decode_lacp_psc_set (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_psc_set *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_LACP_PSC_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  pal_mem_set (msg, 0, sizeof (struct hal_msg_lacp_psc_set));

  /* Interface index. */
  TLV_DECODE_GETL (msg->ifindex);
  
  /* PSC type */
  TLV_DECODE_GETL (msg->psc);

  return *pnt - sp;
}

/* 
   Encode lacp agg id message.
*/
int
hal_msg_encode_lacp_id (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_agg_identifier *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_LACP_AGG_ID_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Aggregator name. */
  TLV_ENCODE_PUT (msg->agg_name, HAL_IFNAME_LEN + 1);

  /* Aggregator interface index. */
  TLV_ENCODE_PUTL(msg->agg_ifindex);

  return *pnt - sp;
}

/* 
   Decode lacp agg id message. 
*/
int
hal_msg_decode_lacp_id (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_agg_identifier *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_LACP_AGG_ID_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  pal_mem_set (msg, 0, sizeof (struct hal_msg_lacp_agg_identifier));

  /* Aggregator interface name. */
  TLV_DECODE_GET (msg->agg_name, HAL_IFNAME_LEN + 1);

  /* Aggregator interface index. */
  TLV_DECODE_GETL (msg->agg_ifindex);

  return *pnt - sp;
}

/* 
   Encode lacp agg add message.
*/
int
hal_msg_encode_lacp_agg_add (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_agg_add *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_LACP_AGG_ADD_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Aggregator name. */
  TLV_ENCODE_PUT (msg->agg_name, HAL_IFNAME_LEN + 1);

  /* Aggregator mac address. */
  TLV_ENCODE_PUT (msg->agg_mac, HAL_HW_LENGTH);

  /* Aggregator type. */
  TLV_ENCODE_PUTL(msg->agg_type);

  return *pnt - sp;
}

/* 
   Decode lacp agg add message. 
*/
int
hal_msg_decode_lacp_agg_add (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_agg_add *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_LACP_AGG_ADD_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  pal_mem_set (msg, 0, sizeof (struct hal_msg_lacp_agg_add));

  /* Aggregator interface name. */
  TLV_DECODE_GET (msg->agg_name, HAL_IFNAME_LEN + 1);

  /* Mac address. */
  TLV_DECODE_GET  (msg->agg_mac, HAL_HW_LENGTH);

  /* Aggregator type. */
  TLV_DECODE_GETL (msg->agg_type);

  return *pnt - sp;
}

/* 
   Encode lacp agg port add/delete message.
*/
int
hal_msg_encode_lacp_mux (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_mux *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_LACP_MUX_ADD_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Aggregator name. */
  TLV_ENCODE_PUT (msg->agg_name, HAL_IFNAME_LEN + 1);

  /* Aggregator interface index. */
  TLV_ENCODE_PUTL(msg->agg_ifindex);

  /* Port name. */
  TLV_ENCODE_PUT (msg->port_name, HAL_IFNAME_LEN + 1);

  /* Port interface index. */
  TLV_ENCODE_PUTL(msg->port_ifindex);

  return *pnt - sp;
}

/* 
   Decode lacp agg add message. 
*/
int
hal_msg_decode_lacp_mux (u_char **pnt, u_int32_t *size, struct hal_msg_lacp_mux *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_LACP_MUX_ADD_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  pal_mem_set (msg, 0, sizeof (struct hal_msg_lacp_mux));

  /* Aggregator interface name. */
  TLV_DECODE_GET (msg->agg_name, HAL_IFNAME_LEN + 1);

  /* Aggregator interface index. */
  TLV_DECODE_GETL (msg->agg_ifindex);

  /* Aggregator interface name. */
  TLV_DECODE_GET (msg->port_name, HAL_IFNAME_LEN + 1);

  /* Aggregator interface index. */
  TLV_DECODE_GETL (msg->port_ifindex);

  return *pnt - sp;
}
#endif /* HAVE_LACPD */

#ifdef HAVE_L2
/* 
   Encode igmp snooping entry message.
*/
int
hal_msg_encode_igs_entry(u_char **pnt, u_int32_t *size, struct hal_msg_igmp_snoop_entry *msg)
{
  unsigned char *sp = *pnt;
  u_int16_t index;

  /* Check size. */
  if (*size < HAL_MSG_IGMP_SNOOP_ENTRY_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_ENCODE_PUT (msg->bridge_name, HAL_BRIDGE_NAME_LEN + 1);

  /* Source address. */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->src);

  /* Group address. */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->group);

  /* IS_EXCLUDE */
  TLV_ENCODE_PUTW (msg->is_exclude);
  
  /* Vlan id. */
  TLV_ENCODE_PUTW (msg->vid);
   
  /* Number of ifindexes. */
  TLV_ENCODE_PUTL (msg->count);
  
  /* ifindexes. */
  for (index = 0; index < msg->count; index++)
    {
      /* Vlan Id */
      TLV_ENCODE_PUTL (msg->ifindexes[index]);
    }

  return *pnt - sp;
}

int
hal_msg_decode_igs_entry(u_char **pnt, u_int32_t *size, struct hal_msg_igmp_snoop_entry *msg,
                         void * (*mem_alloc)(int size,int mtype))
{
  unsigned char *sp = *pnt;
  u_int16_t index;

  /* Check size. */
  if (*size < HAL_MSG_IGMP_SNOOP_ENTRY_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_DECODE_GET (msg->bridge_name, HAL_BRIDGE_NAME_LEN + 1);

  /* Source address. */
  TLV_DECODE_GET_IN4_ADDR (&msg->src);

  /* Group address. */
  TLV_DECODE_GET_IN4_ADDR (&msg->group);

  /* IS_EXCLUDE */
  TLV_DECODE_GETW (msg->is_exclude);

  /* Vlan id. */
  TLV_DECODE_GETW (msg->vid);

  /* Number of ifindexes. */
  TLV_DECODE_GETL (msg->count);

  /*
    If ifindexes count is zero or no memory allocation 
    function was provided return here.
  */
  if((!msg->count) ||(!mem_alloc))
    return *pnt - sp;

  msg->ifindexes = (u_int32_t*)mem_alloc(msg->count * sizeof(u_int32_t *), MTYPE_NSM_HAL_TLV_DECODE);
  if(!msg->ifindexes)
    return HAL_MSG_MEM_ALLOC_ERROR;
  
  /* ifindexes. */
  for (index = 0; index < msg->count; index++)
    {
      /* Interface index */
      TLV_DECODE_GETL (msg->ifindexes[index]);
    }
  return *pnt - sp;
}

/* 
   Encode igmp snooping enable message.
*/
int
hal_msg_encode_igs_bridge(u_char **pnt, u_int32_t *size, struct hal_msg_igs_bridge *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_BRIDGE_IGS_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_ENCODE_PUT (msg->bridge_name, HAL_BRIDGE_NAME_LEN + 1);

  return *pnt - sp;
}

/* 
   Decode igmp snooping enable message. 
*/
int
hal_msg_decode_igs_bridge(u_char **pnt, u_int32_t *size, struct hal_msg_igs_bridge *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_BRIDGE_IGS_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  pal_mem_set (msg, 0, sizeof (struct hal_msg_igs_bridge));

  /* Bridge name. */
  TLV_DECODE_GET (msg->bridge_name, HAL_BRIDGE_NAME_LEN + 1);

  return *pnt - sp;
}

#ifdef HAVE_MLD_SNOOP
/*
   Encode mld snooping enable message.
*/
int
hal_msg_encode_mlds_bridge(u_char **pnt, u_int32_t *size,
                           struct hal_msg_mlds_bridge *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_BRIDGE_MLDS_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_ENCODE_PUT (msg->bridge_name, HAL_BRIDGE_NAME_LEN + 1);

  return *pnt - sp;
}

/* 
   Encode mld snooping entry message.
*/
int
hal_msg_encode_mlds_entry (u_char **pnt, u_int32_t *size,
                           struct hal_msg_mld_snoop_entry *msg)
{
  unsigned char *sp = *pnt;
  u_int16_t index;

  /* Check size. */
  if (*size < HAL_MSG_MLD_SNOOP_ENTRY_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_ENCODE_PUT (msg->bridge_name, HAL_BRIDGE_NAME_LEN + 1);

  /* Source address. */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->src);

  /* Group address. */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->group);

  /* Is-Exclude */
  TLV_ENCODE_PUTW (msg->is_exclude);

  /* Vlan id. */
  TLV_ENCODE_PUTW (msg->vid);
   
  /* Number of ifindexes. */
  TLV_ENCODE_PUTL (msg->count);
  
  /* ifindexes. */
  for (index = 0; index < msg->count; index++)
    {
      /* Vlan Id */
      TLV_ENCODE_PUTL (msg->ifindexes[index]);
    }

  return *pnt - sp;
}

int
hal_msg_decode_mlds_entry (u_char **pnt, u_int32_t *size,
                           struct hal_msg_mld_snoop_entry *msg,
                           void * (*mem_alloc)(int size,int mtype))
{
  unsigned char *sp = *pnt;
  u_int16_t index;

  /* Check size. */
  if (*size < HAL_MSG_MLD_SNOOP_ENTRY_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Bridge name. */
  TLV_DECODE_GET (msg->bridge_name, HAL_BRIDGE_NAME_LEN + 1);

  /* Source address. */
  TLV_DECODE_GET_IN6_ADDR (&msg->src);

  /* Group address. */
  TLV_DECODE_GET_IN6_ADDR (&msg->group);

  /* Is-exclude. */
  TLV_DECODE_GETW (msg->is_exclude);

  /* Vlan id. */
  TLV_DECODE_GETW (msg->vid);

  /* Number of ifindexes. */
  TLV_DECODE_GETL (msg->count);

  /*
    If ifindexes count is zero or no memory allocation 
    function was provided return here.
  */
  if((!msg->count) ||(!mem_alloc))
    return *pnt - sp;

  msg->ifindexes = (u_int32_t *)mem_alloc(msg->count * sizeof(u_int32_t), MTYPE_NSM_HAL_TLV_DECODE);

  if(!msg->ifindexes)
    return HAL_MSG_MEM_ALLOC_ERROR;

  /* ifindexes. */
  for (index = 0; index < msg->count; index++)
    {
      /* Interface index */
      TLV_DECODE_GETL (msg->ifindexes[index]);
    }
  return *pnt - sp;
}
/*
   Decode mld snooping enable message.
*/
int
hal_msg_decode_mlds_bridge(u_char **pnt, u_int32_t *size, 
                           struct hal_msg_mlds_bridge *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_BRIDGE_MLDS_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  pal_mem_set (msg, 0, sizeof (struct hal_msg_mlds_bridge));

  /* Bridge name. */
  TLV_DECODE_GET (msg->bridge_name, HAL_BRIDGE_NAME_LEN + 1);

  return *pnt - sp;
}

#endif /* HAVE_MLD_SNOOP */

/* 
   Encode l2 fdb resp.
*/
int
hal_msg_encode_l2_fdb_resp (u_char **pnt, u_int32_t *size, struct hal_msg_l2_fdb_entry_resp *msg)
{
  unsigned char *sp = *pnt;
  u_int16_t index;

  /* Check size. */
  if (*size < HAL_MSG_L2_FDB_ENTRY_RESP_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Entry count. */
  TLV_ENCODE_PUTL (msg->count);
  
  /* l2 entries . */
  for (index = 0; index < msg->count; index++)
    {
      /* Vlan Id */
      TLV_ENCODE_PUTW (msg->fdb_entry[index].vid);
       
      /* Arp ageing timer */
      TLV_ENCODE_PUTL (msg->fdb_entry[index].ageing_timer_value);

      /* Mac address. */
      TLV_ENCODE_PUT (msg->fdb_entry[index].mac_addr, HAL_HW_LENGTH);

      /* Num */
      TLV_ENCODE_PUTL (msg->fdb_entry[index].ageing_timer_value);

      /* is local */
      TLV_ENCODE_PUTC (msg->fdb_entry[index].is_local);

      /* is static */
      TLV_ENCODE_PUTC (msg->fdb_entry[index].is_static);

      /* is forward */
      TLV_ENCODE_PUTC (msg->fdb_entry[index].is_forward);

      /* Interface */
      TLV_ENCODE_PUTL (msg->fdb_entry[index].port);
    }

  return *pnt - sp;
}

/* 
   Decode l2 fdb response . 
*/
int
hal_msg_decode_l2_fdb_resp (u_char **pnt, u_int32_t *size, struct hal_msg_l2_fdb_entry_resp *msg)
{
  unsigned char *sp = *pnt;
  int index;

  /* Check size. */
  if (*size < HAL_MSG_L2_FDB_ENTRY_RESP_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Entry count. */
  TLV_DECODE_GETL(msg->count);

  if(msg->count > HAL_MAX_L2_FDB_ENTRIES)
    return -1; 

  /* arp entries . */
  for (index = 0; index < msg->count; index++)
    {
      /* Vlan Id */
      TLV_DECODE_GETW (msg->fdb_entry[index].vid);
       
      /* Arp ageing timer */
      TLV_DECODE_GETL (msg->fdb_entry[index].ageing_timer_value);

      /* Mac address. */
      TLV_DECODE_GET  (msg->fdb_entry[index].mac_addr, HAL_HW_LENGTH);

      /* Num */
      TLV_DECODE_GETL (msg->fdb_entry[index].ageing_timer_value);

      /* is local */
      TLV_DECODE_GETC (msg->fdb_entry[index].is_local);

      /* is static */
      TLV_DECODE_GETC (msg->fdb_entry[index].is_static);

      /* is forward */
      TLV_DECODE_GETC (msg->fdb_entry[index].is_forward);

      /* Interface */
      TLV_DECODE_GETL (msg->fdb_entry[index].port);
    }

  return *pnt - sp;
}

/* 
   Encode l2 fdb request.
*/
int
hal_msg_encode_l2_fdb_req (u_char **pnt, u_int32_t *size, struct hal_msg_l2_fdb_entry_req *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_L2_FDB_ENTRY_REQ_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Entry count. */
  TLV_ENCODE_PUTL (msg->count);
  
  /* Request Type. */
  TLV_ENCODE_PUTC (msg->req_type);
  
  /* Vlan Id */
  TLV_ENCODE_PUTW (msg->start_fdb_entry.vid);
       
  /* Arp ageing timer */
  TLV_ENCODE_PUTL (msg->start_fdb_entry.ageing_timer_value);

  /* Mac address. */
  TLV_ENCODE_PUT (msg->start_fdb_entry.mac_addr, HAL_HW_LENGTH);

  /* Num */
  TLV_ENCODE_PUTL (msg->start_fdb_entry.ageing_timer_value);

  /* is local */
  TLV_ENCODE_PUTC (msg->start_fdb_entry.is_local);

  /* is static */
  TLV_ENCODE_PUTC (msg->start_fdb_entry.is_static);

  /* is forward */
  TLV_ENCODE_PUTC (msg->start_fdb_entry.is_forward);

  /* Interface */
  TLV_ENCODE_PUTL (msg->start_fdb_entry.port);

  return *pnt - sp;
}

/* 
   Decode l2 fdb request. 
*/
int
hal_msg_decode_l2_fdb_req (u_char **pnt, u_int32_t *size, struct hal_msg_l2_fdb_entry_req *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_L2_FDB_ENTRY_REQ_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Entry count. */
  TLV_DECODE_GETL(msg->count);
 
  /* Request Type. */
  TLV_DECODE_GETC (msg->req_type);
  
  /* Vlan Id */
  TLV_DECODE_GETW (msg->start_fdb_entry.vid);
       
  /* Arp ageing timer */
  TLV_DECODE_GETL (msg->start_fdb_entry.ageing_timer_value);

  /* Mac address. */
  TLV_DECODE_GET (msg->start_fdb_entry.mac_addr, HAL_HW_LENGTH);

  /* Num */
  TLV_DECODE_GETL (msg->start_fdb_entry.ageing_timer_value);

  /* is local */
  TLV_DECODE_GETC (msg->start_fdb_entry.is_local);

  /* is static */
  TLV_DECODE_GETC (msg->start_fdb_entry.is_static);

  /* is forward */
  TLV_DECODE_GETC (msg->start_fdb_entry.is_forward);

  /* Interface */
  TLV_DECODE_GETL (msg->start_fdb_entry.port);

  return *pnt - sp;
}
#endif /* HAVE_L2 */

#ifdef HAVE_ONMD
/*
   Encode Efm Error Frames req..
*/
int
hal_msg_encode_err_frames_req (u_char **pnt, u_int32_t *size, u_int32_t *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < sizeof(u_int32_t))
    return HAL_MSG_PKT_TOO_SMALL;

  /* encode the index */
  TLV_ENCODE_PUTL (*msg);

  return *pnt - sp;
}

/*
   Decode Efm Error Frames req..
*/
int
hal_msg_decode_err_frames_req (u_char **pnt, u_int32_t *size, u_int32_t *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < sizeof(HAL_MSG_EFM_ERR_FRAME_REQ_SIZE))
    return HAL_MSG_PKT_TOO_SMALL;

  /* encode the index */
  TLV_DECODE_GETL (*msg);

  return *pnt - sp;
}

/*Encode Efm Error Frames resp*/
int
hal_msg_encode_err_frames_resp (u_char **pnt, u_int32_t *size, 
                                struct hal_msg_efm_err_frames *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_EFM_ERR_FRAME_RESP_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Encode the index  */
  TLV_ENCODE_PUTL (msg->index);
 
  /* Encode err frames count*/
  TLV_ENCODE_PUT (&msg->err_frames, sizeof(ut_int64_t));

  return *pnt - sp;
}

/*Decode Efm Error Frames resp*/
int
hal_msg_decode_err_frames_resp(u_char **pnt, u_int32_t *size, 
                               struct hal_msg_efm_err_frames_resp *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_EFM_ERR_FRAME_RESP_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Get the index */
  TLV_DECODE_GETL (msg->index);
 
  /*Get the error frame count*/
  TLV_DECODE_GET (msg->err_frames, sizeof(ut_int64_t));
 
  return *pnt - sp;
}


/*
   Encode Efm Error Frame Seconds req..
*/
int
hal_msg_encode_err_frame_secs_req (u_char **pnt, u_int32_t *size, u_int32_t *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < sizeof(u_int32_t))
    return HAL_MSG_PKT_TOO_SMALL;

  /* encode the index */
  TLV_ENCODE_PUTL (*msg);

  return *pnt - sp;
}

/*
   Decode Efm Error Frames Seconds req..
*/
int
hal_msg_decode_err_frame_secs_req (u_char **pnt, u_int32_t *size, u_int32_t *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < sizeof(HAL_MSG_EFM_ERR_FRAME_REQ_SIZE))
    return HAL_MSG_PKT_TOO_SMALL;

  /* encode the index */
  TLV_DECODE_GETL (*msg);

  return *pnt - sp;
}

/*Encode Efm Error Frame Seconds resp*/
int
hal_msg_encode_err_frame_secs_resp (u_char **pnt, u_int32_t *size, 
                                    struct hal_msg_efm_err_frame_stat *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_EFM_ERR_FRAME_RESP_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Encode the index  */
  TLV_ENCODE_PUTL (msg->index);
 
  /* Encode err frames count*/
  TLV_ENCODE_PUTL (msg->err_frames);

  return *pnt - sp;
}

/*Decode Efm Error Frame Seconds resp*/
int
hal_msg_decode_err_frame_secs_resp (u_char **pnt, u_int32_t *size, 
                                struct hal_msg_efm_err_frame_secs_resp *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_EFM_ERR_FRAME_RESP_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Get the index */
  TLV_DECODE_GETL (msg->index);
 
  /*Get the error frame count*/
  TLV_DECODE_GET (msg->err_frame_secs, sizeof(u_int32_t));
 
  return *pnt - sp;
}
#endif /*HAVE_ONMD*/

#ifdef HAVE_VLAN_CLASS
/*
  Encode vlan classifier rule message.
*/
int
hal_msg_encode_vlan_classifier_rule(u_char **pnt, u_int32_t *size, struct hal_msg_vlan_classifier_rule *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_VLAN_CLASS_RULE_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Classifier type . */
  TLV_ENCODE_PUTL (msg->type);

  /* Vlan id. */
  TLV_ENCODE_PUTW (msg->vlan_id);

  /* If Index */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Rule ref count*/
  TLV_ENCODE_PUTL (msg->refcount);

  switch (msg->type)
    {  
    case HAL_VLAN_CLASSIFIER_MAC: 
      /* Mac address. */
      TLV_ENCODE_PUT (msg->u.hw_addr, HAL_HW_LENGTH);
      break;
    case HAL_VLAN_CLASSIFIER_PROTOCOL:
      /* Protocol id (ether type) */
      TLV_ENCODE_PUTW (msg->u.protocol.ether_type);
      /* Packet encapsulation. */  
      TLV_ENCODE_PUTL (msg->u.protocol.encaps);
      break;
    case HAL_VLAN_CLASSIFIER_IPV4:
      /* Prefix. */
      TLV_ENCODE_PUT_IN4_ADDR (&msg->u.ipv4.addr);
      /* Masklen. */
      TLV_ENCODE_PUTC (msg->u.ipv4.masklen);
      break;
    } 
  return *pnt - sp;
}

/*
  Decode vlan classifier rule message.
*/
int
hal_msg_decode_vlan_classifier_rule(u_char **pnt, u_int32_t *size, struct hal_msg_vlan_classifier_rule *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_VLAN_CLASS_RULE_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Classifier type . */
  TLV_DECODE_GETL (msg->type);

  /* Vlan id. */
  TLV_DECODE_GETW (msg->vlan_id);

  /* If Index */
  TLV_DECODE_GETL (msg->ifindex);

  /* Rule reference count. */
  TLV_DECODE_GETL (msg->refcount);
   
  switch (msg->type)
    { 
    case HAL_VLAN_CLASSIFIER_MAC:
      /* Mac address. */
      TLV_DECODE_GET (msg->u.hw_addr, HAL_HW_LENGTH);
      break;
    case HAL_VLAN_CLASSIFIER_PROTOCOL:
      /* Protocol id (ether type) */
      TLV_DECODE_GETW (msg->u.protocol.ether_type);
      /* Packet encapsulation. */  
      TLV_DECODE_GETL (msg->u.protocol.encaps);
      break;
    case HAL_VLAN_CLASSIFIER_IPV4:
      /* Prefix. */
      TLV_DECODE_GET_IN4_ADDR (&msg->u.ipv4.addr);
      /* Masklen. */
      TLV_DECODE_GETC (msg->u.ipv4.masklen);
      break;
    }
  return *pnt - sp;
}
#endif /* HAVE_VLAN_CLASS */
#ifdef HAVE_L3
/* 
   Encode FIB if bind/unbind msg.
*/
int
hal_msg_encode_if_fib_bind_unbind (u_char **pnt, u_int32_t *size, 
    struct hal_msg_if_fib_bind_unbind *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IF_FIB_BIND_UNBIND_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Ifindex. */
  TLV_ENCODE_PUTL (msg->ifindex);
  
  /* FIB id. */
  TLV_ENCODE_PUTL (msg->fib_id);

  return *pnt - sp;
}

/* 
   Decode FIB if bind/unbind msg.
*/
int
hal_msg_decode_if_fib_bind_unbind (u_char **pnt, u_int32_t *size, 
    struct hal_msg_if_fib_bind_unbind *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IF_FIB_BIND_UNBIND_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Ifindex. */
  TLV_DECODE_GETL (msg->ifindex);
  
  /* FIB id. */
  TLV_DECODE_GETL (msg->fib_id);

  return *pnt - sp;
}

/* 
   Encode arp cache resp.
*/
int
hal_msg_encode_arp_cache_resp (u_char **pnt, u_int32_t *size, struct hal_msg_arp_cache_resp *msg)
{
  unsigned char *sp = *pnt;
  u_int16_t i;

  /* Check size. */
  if (*size < HAL_MSG_ARP_CACHE_RESP_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Entry count. */
  TLV_ENCODE_PUTL (msg->count);
  
  /* arp entries . */
  for (i = 0; i < msg->count; i++)
    {
      /* Arp ip address. */
      TLV_ENCODE_PUT_IN4_ADDR (&msg->cache[i].ip_addr);

      /* Mac address. */
      TLV_ENCODE_PUT (msg->cache[i].mac_addr, HAL_HW_LENGTH);
  
      /* If Index */
      TLV_ENCODE_PUTL (msg->cache[i].ifindex);

      /* Flags */
      TLV_ENCODE_PUTL (msg->cache[i].static_flag);
    }
  return *pnt - sp;
}

/* 
   Decode arp cache resp. 
*/
int
hal_msg_decode_arp_cache_resp (u_char **pnt, u_int32_t *size, struct hal_msg_arp_cache_resp *msg)
{
  unsigned char *sp = *pnt;
  int i;

  /* Check size. */
  if (*size < HAL_MSG_ARP_CACHE_RESP_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Entry count. */
  TLV_DECODE_GETL(msg->count);

  if(msg->count > HAL_ARP_CACHE_GET_COUNT)
    return -1; 

  /* arp entries . */
  for (i = 0; i < msg->count; i++)
    {
      /* Arp ip address. */
      TLV_DECODE_GET_IN4_ADDR(&msg->cache[i].ip_addr);

      /* Mac address. */
      TLV_DECODE_GET(msg->cache[i].mac_addr, HAL_HW_LENGTH);
  
      /* If Index */
      TLV_DECODE_GETL(msg->cache[i].ifindex);

      /* Flags */
      TLV_DECODE_GETL(msg->cache[i].static_flag);
    }
  return *pnt - sp;
}

/* 
   Encode arp cache request.
*/
int
hal_msg_encode_arp_cache_req (u_char **pnt, u_int32_t *size, struct hal_msg_arp_cache_req *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_ARP_CACHE_REQ_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* FIB id. */
  TLV_ENCODE_PUTW (msg->fib_id);

  /* Arp ip address. */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->ip_addr);

  /* Entry count. */
  TLV_ENCODE_PUTL (msg->count);
  return *pnt - sp;
}

/* 
   Decode arp cache request. 
*/
int
hal_msg_decode_arp_cache_req (u_char **pnt, u_int32_t *size, struct hal_msg_arp_cache_req *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_ARP_CACHE_REQ_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* FIB Id */
  TLV_DECODE_GETW(msg->fib_id);

  /* Arp ip address. */
  TLV_DECODE_GET_IN4_ADDR(&msg->ip_addr);
  /* Entry count. */

  TLV_DECODE_GETL(msg->count);

  return *pnt - sp;
}

/*
  Encode ipv4 address message.
*/
int
hal_msg_encode_ipv4_addr(u_char **pnt, u_int32_t *size, struct hal_msg_if_ipv4_addr *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV4_ADDR_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Interface name. */
  TLV_ENCODE_PUT (msg->name, HAL_IFNAME_LEN + 1);

  /* If Index */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Prefix. */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->addr);

  /* Masklen. */
  TLV_ENCODE_PUTC (msg->ipmask);

  return *pnt - sp;
}

/*
  Decode ipv4 address message.
*/
int
hal_msg_decode_ipv4_addr(u_char **pnt, u_int32_t *size, struct hal_msg_if_ipv4_addr *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV4_ADDR_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Interface name. */
  TLV_DECODE_GET (msg->name, HAL_IFNAME_LEN + 1);

  /* If Index */
  TLV_DECODE_GETL (msg->ifindex);

  /* Get prefix. */
  TLV_DECODE_GET_IN4_ADDR (&msg->addr);

  /* Masklen. */
  TLV_DECODE_GETC (msg->ipmask);

  return *pnt - sp;
}
#ifdef HAVE_IPV6
/* 
   Encode ipv6 nbr cache resp.
*/
int
hal_msg_encode_ipv6_nbr_cache_resp (u_char **pnt, u_int32_t *size, 
                                    struct hal_msg_ipv6_nbr_cache_resp *msg)
{
  unsigned char *sp = *pnt;
  u_int16_t i;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_NBR_CACHE_RESP_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Entry count. */
  TLV_ENCODE_PUTL (msg->count);
  
  /* arp entries . */
  for (i = 0; i < msg->count; i++)
    {
      /* Arp ip address. */
      TLV_ENCODE_PUT_IN6_ADDR (&msg->cache[i].addr);

      /* Mac address. */
      TLV_ENCODE_PUT (msg->cache[i].mac_addr, HAL_HW_LENGTH);
  
      /* If Index */
      TLV_ENCODE_PUTL (msg->cache[i].ifindex);

      /* Flags */
      TLV_ENCODE_PUTL (msg->cache[i].static_flag);
    }
  return *pnt - sp;
}

/* 
   Decode arp cache resp. 
*/
int
hal_msg_decode_ipv6_nbr_cache_resp (u_char **pnt, u_int32_t *size, 
                                    struct hal_msg_ipv6_nbr_cache_resp *msg)
{
  unsigned char *sp = *pnt;
  int i;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_NBR_CACHE_RESP_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Entry count. */
  TLV_DECODE_GETL(msg->count);

  if(msg->count > HAL_IPV6_NBR_CACHE_GET_COUNT)
    return -1; 

  /* arp entries . */
  for (i = 0; i < msg->count; i++)
    {
      /* IPV6 address. */
      TLV_DECODE_GET_IN6_ADDR(&msg->cache[i].addr);

      /* Mac address. */
      TLV_DECODE_GET(msg->cache[i].mac_addr, HAL_HW_LENGTH);
  
      /* If Index */
      TLV_DECODE_GETL(msg->cache[i].ifindex);

      /* Flags */
      TLV_DECODE_GETL(msg->cache[i].static_flag);
    }
  return *pnt - sp;
}

/* 
   Encode ipv6 nbr cache request.
*/
int
hal_msg_encode_ipv6_nbr_cache_req (u_char **pnt, u_int32_t *size,
                                   struct hal_msg_ipv6_nbr_cache_req *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_NBR_CACHE_REQ_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* FIB id. */
  TLV_ENCODE_PUTW (msg->fib_id);

  /* IPV6 address. */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->addr);

  /* Entry count. */
  TLV_ENCODE_PUTL (msg->count);
  return *pnt - sp;
}

/* 
   Decode ipv6 nbr cache request. 
*/
int
hal_msg_decode_ipv6_nbr_cache_req (u_char **pnt, u_int32_t *size, 
                                   struct hal_msg_ipv6_nbr_cache_req *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_ARP_CACHE_REQ_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  TLV_DECODE_GETW(msg->fib_id);

  /* Arp ip address. */
  TLV_DECODE_GET_IN6_ADDR(&msg->addr);
  /* Entry count. */

  TLV_DECODE_GETL(msg->count);

  return *pnt - sp;
}


/*
  Encode ipv6 address message.
*/
int
hal_msg_encode_ipv6_addr(u_char **pnt, u_int32_t *size, struct hal_msg_if_ipv6_addr *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_ADDR_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Interface name. */
  TLV_ENCODE_PUT (msg->name, HAL_IFNAME_LEN + 1);

  /* If Index */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Prefix. */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->addr);

  /* Masklen. */
  TLV_ENCODE_PUTC (msg->ipmask);

  /* Flags. */
  TLV_ENCODE_PUTC (msg->flags);

  return *pnt - sp;
}

/*
  Decode ipv6 address message.
*/
int
hal_msg_decode_ipv6_addr(u_char **pnt, u_int32_t *size, struct hal_msg_if_ipv6_addr *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_ADDR_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Interface name. */
  TLV_DECODE_GET (msg->name, HAL_IFNAME_LEN + 1);

  /* If Index */
  TLV_DECODE_GETL (msg->ifindex);

  /* Get prefix. */
  TLV_DECODE_GET_IN6_ADDR (&msg->addr);

  /* Masklen. */
  TLV_DECODE_GETC (msg->ipmask);

  /* Flags. */
  TLV_DECODE_GETC(msg->flags);

  return *pnt - sp;
}
#endif /* HAVE_IPV6 */

/*
  Encode route message.
*/
int
hal_msg_encode_ipv4_route (u_char **pnt, u_int32_t *size, struct hal_msg_ipv4uc_route *msg)
{
  unsigned char *sp = *pnt;
  u_int16_t i;

  /* Check size. */
  if (*size < HAL_MSG_IPV4_ROUTE_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* FIB id. */
  TLV_ENCODE_PUTW (msg->fib_id);

  /* Prefix. */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->addr);

  /* Masklen. */
  TLV_ENCODE_PUTC (msg->masklen);

  for (i = 0; i < msg->num; i++)
    {
      hal_msg_encode_tlv (pnt, size, HAL_MSG_CINDEX_NEXTHOP, 13);

      TLV_ENCODE_PUTL (msg->nh[i].id);
      TLV_ENCODE_PUTC (msg->nh[i].type);
      TLV_ENCODE_PUTL (msg->nh[i].egressIfindex);
      TLV_ENCODE_PUT_IN4_ADDR (&msg->nh[i].nexthopIP);
    }

  return *pnt - sp;
}

#ifdef HAVE_IPV6
/*
  Encode IPV6 route message.
*/
int
hal_msg_encode_ipv6_route (u_char **pnt, u_int32_t *size, struct hal_msg_ipv6uc_route *msg)
{
  unsigned char *sp = *pnt;
  u_int16_t i;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_ROUTE_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* FIB id. */
  TLV_ENCODE_PUTW (msg->fib_id);

  /* Prefix. */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->addr);

  /* Masklen. */
  TLV_ENCODE_PUTC (msg->masklen);

  for (i = 0; i < msg->num; i++)
    {
      hal_msg_encode_tlv (pnt, size, HAL_MSG_CINDEX_NEXTHOP, 13);

      TLV_ENCODE_PUTL (msg->nh[i].id);
      TLV_ENCODE_PUTC (msg->nh[i].type);
      TLV_ENCODE_PUTL (msg->nh[i].egressIfindex);
      TLV_ENCODE_PUT_IN6_ADDR (&msg->nh[i].nexthopIP);
    }

  return *pnt - sp;
}
#endif /* HAVE_IPV6 */

/*
  Decode route message. 
*/
int
hal_msg_decode_ipv4_route (u_char **pnt, u_int32_t *size, struct hal_msg_ipv4uc_route *msg)
{
  unsigned char *sp = *pnt;
  struct hal_msg_tlv_header tlv;
  int num;

  /* Check size. */
  if (*size < HAL_MSG_IPV4_ROUTE_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* FIB id. */
  TLV_DECODE_GETW (msg->fib_id);

  /* Get prefix. */
  TLV_DECODE_GET_IN4_ADDR (&msg->addr);

  /* Masklen. */
  TLV_DECODE_GETC (msg->masklen);

  /* Decode nexthops. */
  num = 0;
  while (*size)
    {
      if (*size < HAL_MSG_TLV_HEADER_SIZE)
        return HAL_MSG_PKT_TOO_SMALL;

      HAL_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case HAL_MSG_CINDEX_NEXTHOP:
          TLV_DECODE_GETL (msg->nh[num].id);
          TLV_DECODE_GETC (msg->nh[num].type);
          TLV_DECODE_GETL (msg->nh[num].egressIfindex);
          TLV_DECODE_GET_IN4_ADDR (&msg->nh[num].nexthopIP);
          num++;
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;  
        }
    }

  /* Total nexthops. */
  msg->num = num;

  return *pnt - sp;
}

#ifdef HAVE_IPV6
/*
  Decode IPV6 route message. 
*/
int
hal_msg_decode_ipv6_route (u_char **pnt, u_int32_t *size, struct hal_msg_ipv6uc_route *msg)
{
  unsigned char *sp = *pnt;
  struct hal_msg_tlv_header tlv;
  int num;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_ROUTE_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* FIB id. */
  TLV_DECODE_GETW (msg->fib_id);

  /* Get prefix. */
  TLV_DECODE_GET_IN6_ADDR (&msg->addr);

  /* Masklen. */
  TLV_DECODE_GETC (msg->masklen);

  /* Decode nexthops. */
  num = 0;
  while (*size)
    {
      if (*size < HAL_MSG_TLV_HEADER_SIZE)
        return HAL_MSG_PKT_TOO_SMALL;

      HAL_DECODE_TLV_HEADER (tlv);

      switch (tlv.type)
        {
        case HAL_MSG_CINDEX_NEXTHOP:
          TLV_DECODE_GETL (msg->nh[num].id);
          TLV_DECODE_GETC (msg->nh[num].type);
          TLV_DECODE_GETL (msg->nh[num].egressIfindex);
          TLV_DECODE_GET_IN6_ADDR (&msg->nh[num].nexthopIP);
          num++;
          break;
        default:
          TLV_DECODE_SKIP (tlv.length);
          break;  
        }
    }

  /* Total nexthops. */
  msg->num = num;

  return *pnt - sp;
}
#endif /* HAVE_IPV6 */

#ifdef HAVE_MCAST_IPV4
/*
  Encode vif add message.
*/
int
hal_msg_encode_ipv4_vif_add (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv4mc_vif_add *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV4_VIF_ADD_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Vif Index */
  TLV_ENCODE_PUTL (msg->vif_index);

  /* If Index */
  TLV_ENCODE_PUTL (msg->ifindex);

  /* Local Address. */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->loc_addr);

  /* Remote Address. */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->rmt_addr);

  /* Flags */
  TLV_ENCODE_PUTW (msg->flags);

  return *pnt - sp;
}

/*
  Decode vif add message.
*/
int
hal_msg_decode_ipv4_vif_add (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv4mc_vif_add *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV4_VIF_ADD_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Vif Index */
  TLV_DECODE_GETL (msg->vif_index);

  /* If Index */
  TLV_DECODE_GETL (msg->ifindex);

  /* Local Address. */
  TLV_DECODE_GET_IN4_ADDR (&msg->loc_addr);

  /* Remote Address. */
  TLV_DECODE_GET_IN4_ADDR (&msg->rmt_addr);

  /* Flags */
  TLV_DECODE_GETW (msg->flags);

  return *pnt - sp;
}

/*
  Encode mrt add message.
*/
int
hal_msg_encode_ipv4_mrt_add (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv4mc_mrt_add *msg)
{
  unsigned char *sp = *pnt;
  int i = 0;

  /* Check size. */
  if (*size < (HAL_MSG_IPV4_MRT_ADD_SIZE +
               (msg->num_olist * sizeof (u_char))))
    return HAL_MSG_PKT_TOO_SMALL;

  /* Source Address. */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->src);

  /* Group Address. */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->group);

  /* Iif Vif Index */
  TLV_ENCODE_PUTL (msg->iif_vif);

  /* Length of olist. */
  TLV_ENCODE_PUTL (msg->num_olist);

  /* Olist TTLs */
  while (i < msg->num_olist)
    {
      TLV_ENCODE_PUTC (msg->olist_ttls[i]);
      i++;
    }

  return *pnt - sp;
}

/*
  Decode mrt add message.
*/
int
hal_msg_decode_ipv4_mrt_add (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv4mc_mrt_add *msg)
{
  unsigned char *sp = *pnt;
  int i = 0;

  /* Check size. */
  if (*size < HAL_MSG_IPV4_MRT_ADD_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Source Address. */
  TLV_DECODE_GET_IN4_ADDR (&msg->src);

  /* Group Address. */
  TLV_DECODE_GET_IN4_ADDR (&msg->group);

  /* Iif Vif Index */
  TLV_DECODE_GETL (msg->iif_vif);

  /* Length of olist. */
  TLV_DECODE_GETL (msg->num_olist);

  /* Check minimum required size.  */
  if (*size < (msg->num_olist * sizeof (u_char))) 
    return HAL_MSG_PKT_TOO_SMALL;

  /* Decode olist */
  msg->olist_ttls = XMALLOC (MTYPE_TMP, (msg->num_olist * sizeof (u_char)));
  if (msg->olist_ttls == NULL)
    return -1;
  
  while (i < msg->num_olist)
    {
      TLV_DECODE_GETC (msg->olist_ttls[i]);
      i++;
    }

  return *pnt - sp;
}

/*
  Encode mrt del message.
*/
int
hal_msg_encode_ipv4_mrt_del (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv4mc_mrt_del *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV4_MRT_DEL_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Source Address. */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->src);

  /* Group Address. */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->group);

  return *pnt - sp;
}

/*
  Decode mrt del message.
*/
int
hal_msg_decode_ipv4_mrt_del (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv4mc_mrt_del *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV4_MRT_DEL_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Source Address. */
  TLV_DECODE_GET_IN4_ADDR (&msg->src);

  /* Group Address. */
  TLV_DECODE_GET_IN4_ADDR (&msg->group);

  return *pnt - sp;
}

/*
  Encode sg stat message.
*/
int
hal_msg_encode_ipv4_sg_stat (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv4mc_sg_stat*msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV4_SG_STAT_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Source Address. */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->src);

  /* Group Address. */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->group);

  /* Iif vif */
  TLV_ENCODE_PUTL (msg->iif_vif);

  /* Pktcount */
  TLV_ENCODE_PUTL (msg->pktcnt);

  /* Bytecnt */
  TLV_ENCODE_PUTL (msg->bytecnt);

  /* Wrong if */
  TLV_ENCODE_PUTL (msg->wrong_if);

  return *pnt - sp;
}

/*
  Decode sg stat message.
*/
int
hal_msg_decode_ipv4_sg_stat (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv4mc_sg_stat *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV4_SG_STAT_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Source Address. */
  TLV_DECODE_GET_IN4_ADDR (&msg->src);

  /* Group Address. */
  TLV_DECODE_GET_IN4_ADDR (&msg->group);

  /* Iif vif */
  TLV_DECODE_GETL (msg->iif_vif);

  /* Pktcount */
  TLV_DECODE_GETL (msg->pktcnt);

  /* Bytecount */
  TLV_DECODE_GETL (msg->bytecnt);

  /* Wrong if */
  TLV_DECODE_GETL (msg->wrong_if);

  return *pnt - sp;
}
#endif /* HAVE_MCAST_IPV4 */

#ifdef HAVE_MCAST_IPV6
/*
  Encode vif add message.
*/
int
hal_msg_encode_ipv6_vif_add (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv6mc_vif_add *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_VIF_ADD_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Vif Index */
  TLV_ENCODE_PUTL (msg->vif_index);

  /* Physical Ifindex */
  TLV_ENCODE_PUTL (msg->phy_ifindex);

  /* Flags */
  TLV_ENCODE_PUTW (msg->flags);

  return *pnt - sp;
}

/*
  Decode vif add message.
*/
int
hal_msg_decode_ipv6_vif_add (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv6mc_vif_add *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_VIF_ADD_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Vif Index */
  TLV_DECODE_GETL (msg->vif_index);

  /* Physical Ifindex */
  TLV_DECODE_GETL (msg->phy_ifindex);

  /* Flags */
  TLV_DECODE_GETW (msg->flags);

  return *pnt - sp;
}

/*
  Encode mrt add message.
*/
int
hal_msg_encode_ipv6_mrt_add (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv6mc_mrt_add *msg)
{
  unsigned char *sp = *pnt;
  int i = 0;

  /* Check size. */
  if (*size < (HAL_MSG_IPV6_MRT_ADD_SIZE +
               (msg->num_olist * sizeof (u_int16_t))))
    return HAL_MSG_PKT_TOO_SMALL;

  /* Source Address. */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->src);

  /* Group Address. */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->group);

  /* Iif Vif Index */
  TLV_ENCODE_PUTL (msg->iif_vif);

  /* Num of 32-bit elements in olist bitmap */
  TLV_ENCODE_PUTL (msg->num_olist);

  /* Olist Bitmap */
  while (i < msg->num_olist)
    {
      TLV_ENCODE_PUTW (msg->olist[i]);
      i++;
    }

  return *pnt - sp;
}

/*
  Decode mrt add message.
*/
/* Note that this function allocates memory to hold decoded olist.
 * This memory is expected to be freed by the caller of this function.
 */
int
hal_msg_decode_ipv6_mrt_add (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv6mc_mrt_add *msg)
{
  unsigned char *sp = *pnt;
  int i = 0;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_MRT_ADD_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Source Address. */
  TLV_DECODE_GET_IN6_ADDR (&msg->src);

  /* Group Address. */
  TLV_DECODE_GET_IN6_ADDR (&msg->group);

  /* Iif Vif Index */
  TLV_DECODE_GETL (msg->iif_vif);

  /* Num of 32-bit elements in olist bitmap */
  TLV_DECODE_GETL (msg->num_olist);

  if (msg->num_olist == 0)
    return *pnt - sp;

  /* Check minimum required size.  */
  if (*size < (msg->num_olist * sizeof (u_int16_t))) 
    return HAL_MSG_PKT_TOO_SMALL;

  /* Decode olist */
  msg->olist = XMALLOC (MTYPE_TMP, (msg->num_olist * sizeof (u_int16_t)));
  if (msg->olist == NULL)
    return -1;
  
  while (i < msg->num_olist)
    {
      TLV_DECODE_GETW (msg->olist[i]);
      i++;
    }

  return *pnt - sp;
}

/*
  Encode mrt del message.
*/
int
hal_msg_encode_ipv6_mrt_del (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv6mc_mrt_del *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_MRT_DEL_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Source Address. */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->src);

  /* Group Address. */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->group);

  return *pnt - sp;
}

/*
  Decode mrt del message.
*/
int
hal_msg_decode_ipv6_mrt_del (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv6mc_mrt_del *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_MRT_DEL_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Source Address. */
  TLV_DECODE_GET_IN6_ADDR (&msg->src);

  /* Group Address. */
  TLV_DECODE_GET_IN6_ADDR (&msg->group);

  return *pnt - sp;
}

/*
  Encode sg reply message.
*/
int
hal_msg_encode_ipv6_sg_stat (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv6mc_sg_stat *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_SG_STAT_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Source Address. */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->src);

  /* Group Address. */
  TLV_ENCODE_PUT_IN6_ADDR (&msg->group);

  /* Iif vif */
  TLV_ENCODE_PUTL (msg->iif_vif);

  /* Pktcount */
  TLV_ENCODE_PUTL (msg->pktcnt);

  /* Bytecnt */
  TLV_ENCODE_PUTL (msg->bytecnt);

  /* Wrong if */
  TLV_ENCODE_PUTL (msg->wrong_if);

  return *pnt - sp;
}

/*
  Decode sg stat  message.
*/
int
hal_msg_decode_ipv6_sg_stat (u_char **pnt, u_int32_t *size, 
                             struct hal_msg_ipv6mc_sg_stat *msg)
{
  unsigned char *sp = *pnt;

  /* Check size. */
  if (*size < HAL_MSG_IPV6_SG_STAT_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;

  /* Source Address. */
  TLV_DECODE_GET_IN6_ADDR (&msg->src);

  /* Group Address. */
  TLV_DECODE_GET_IN6_ADDR (&msg->group);

  /* Iif vif */
  TLV_DECODE_GETL (msg->iif_vif);

  /* Pktcount */
  TLV_DECODE_GETL (msg->pktcnt);

  /* Bytecount */
  TLV_DECODE_GETL (msg->bytecnt);

  /* Wrong if */
  TLV_DECODE_GETL (msg->wrong_if);

  return *pnt - sp;
}
#endif /* HAVE_MCAST_IPV6 */

#ifdef HAVE_PBR
int
hal_msg_encode_pbr_ipv4_route (u_char **pnt, u_int32_t *size,
                                struct hal_msg_pbr_ipv4_route *msg)
{
  unsigned char *sp = *pnt;
  u_int16_t i;

  /* Check size */
  if (*size < HAL_MSG_IPV4_ROUTE_SIZE)
    return HAL_MSG_PKT_TOO_SMALL;
 
  /* route map name */
  TLV_ENCODE_PUT (msg->rmap_name, HAL_RMAP_NAME_LEN + 1); 
  /* map sequence num */
  TLV_ENCODE_PUTL (msg->seq_num);
  
  /* Source prefix */
  TLV_ENCODE_PUTC (msg->src_prefix.family);
  TLV_ENCODE_PUTC (msg->src_prefix.prefixlen);
  TLV_ENCODE_PUT_IN4_ADDR (&msg->src_prefix.u.prefix4);

  /* Destination prefix */
  TLV_ENCODE_PUTC (msg->dst_prefix.family);
  TLV_ENCODE_PUTC (msg->dst_prefix.prefixlen);
  TLV_ENCODE_PUT_IN4_ADDR (&msg->dst_prefix.u.prefix4);

  /* L4 protocol */
  for (i = 0; i < CINDEX_SIZE; i++)
    {
      if (CHECK_CINDEX (msg->cindex, i))
        {
          if (*size < HAL_MSG_TLV_HEADER_SIZE)
            return HAL_MSG_PKT_TOO_SMALL;

          switch (i)
            {
              case HAL_MSG_CINDEX_PBR_PROTOCOL:
                hal_msg_encode_tlv (pnt, size, i, 2);
                TLV_ENCODE_PUTL (msg->protocol);
                break;
              case HAL_MSG_CINDEX_PBR_SRC_PORT:
                switch (msg->sport_op)
                  {
                    case EQUAL:
                      hal_msg_encode_tlv (pnt, size, i, 6);
                      TLV_ENCODE_PUTW (msg->sport_op);
                      TLV_ENCODE_PUTL (msg->sport);
                      break;
                    case NOT_EQUAL:
                      hal_msg_encode_tlv (pnt, size, i, 6);
                      TLV_ENCODE_PUTW (msg->sport_op);
                      TLV_ENCODE_PUTL (msg->sport);
                      break;
                    case LESS_THAN:
                      hal_msg_encode_tlv (pnt, size, i, 6);
                      TLV_ENCODE_PUTL (msg->sport_op);
                      TLV_ENCODE_PUTL (msg->sport);
                      break;
                    case GREATER_THAN:
                      hal_msg_encode_tlv (pnt, size, i, 6);
                      TLV_ENCODE_PUTW (msg->sport_op);
                      TLV_ENCODE_PUTL (msg->sport);
                      break;
                    case RANGE:
                      hal_msg_encode_tlv (pnt, size, i, 10);
                      TLV_ENCODE_PUTW (msg->sport_op);
                      TLV_ENCODE_PUTL (msg->sport);
                      TLV_ENCODE_PUTL (msg->sport_low);
                      break;
                  }
                break;

              case HAL_MSG_CINDEX_PBR_DST_PORT:
                switch (msg->dport_op)
                  {
                    case EQUAL:
                      hal_msg_encode_tlv (pnt, size, i, 6);
                      TLV_ENCODE_PUTW (msg->dport_op);
                      TLV_ENCODE_PUTL (msg->dport);
                      break;
                    case NOT_EQUAL:
                      hal_msg_encode_tlv (pnt, size, i, 6);
                      TLV_ENCODE_PUTW (msg->dport_op);
                      TLV_ENCODE_PUTL (msg->dport);
                      break;
                    case LESS_THAN:
                      hal_msg_encode_tlv (pnt, size, i, 6);
                      TLV_ENCODE_PUTW (msg->dport_op);
                      TLV_ENCODE_PUTL (msg->dport);
                      break;
                    case GREATER_THAN:
                      hal_msg_encode_tlv (pnt, size, i, 6);
                      TLV_ENCODE_PUTW (msg->dport_op);
                      TLV_ENCODE_PUTL (msg->dport);
                      break;
                    case RANGE:
                      hal_msg_encode_tlv (pnt, size, i, 10);
                      TLV_ENCODE_PUTW (msg->dport_op);
                      TLV_ENCODE_PUTL (msg->dport);
                      TLV_ENCODE_PUTL (msg->dport_low);
                      break;
                  }
                break;
              case HAL_MSG_CINDEX_PBR_TOS:
                switch (msg->tos)
                  {
                    case TOS_NORMAL_SERVICE:
                    case TOS_MIN_COST:
                    case TOS_MAX_RELIABILITY:
                    case TOS_MAX_THROUGHPUT:
                    case TOS_MIN_DELAT:
                      hal_msg_encode_tlv (pnt, size, i, 1);
                      TLV_ENCODE_PUTC (msg->tos);
                      break;
                  }
                break;
              /* pcp bits */
              case HAL_MSG_CINDEX_PBR_PCP:
                hal_msg_encode_tlv (pnt, size, i, 4);
                TLV_ENCODE_PUTL (msg->pcp);
                break;
              default:
                break;
            }  /* end of switch */
        } /* end of if */
    } /* end of for */
  
  /* nexthop address */
  TLV_ENCODE_PUT_IN4_ADDR (&msg->nexthop);
  
  /* nexthop type primary or secondary */
  TLV_ENCODE_PUTW (msg->nh_type);
  /* interface name */
  TLV_ENCODE_PUT (msg->ifname, HAL_IFNAME_LEN + 1);

  /* Rule type permit/deny */
  TLV_ENCODE_PUTL (msg->rule_type);

  /* interface index */
  TLV_ENCODE_PUTL (msg->if_id);

  return *pnt - sp;
}
#endif /* HAVE_PBR */

#endif /* HAVE_L3 */
