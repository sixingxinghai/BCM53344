/* Copyright 2003  All Rights Reserved. */
#include "pal.h"
#include "lib.h"
#include "if.h"
#include "table.h"
#include "avl_tree.h"
#include "l2_llc.h"

#include "nsm_interface.h"
#include "nsm_bridge.h"
#include "nsm_vlan.h"

#include "hal_types.h"
#include "garp_sock.h"

#include "garp_pdu.h"
#include "garp_gid.h"

#include "gvrp.h"
#include "gvrp_api.h"
#include "gvrp_database.h"
#include "gvrp_debug.h"
#include "gvrp_pdu.h"

const u_char
gvrp_dest_addr[HAL_HW_LENGTH] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x21};

const u_char
pro_gvrp_dest_addr[HAL_HW_LENGTH] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x0D};

static inline void
gvrp_set_octets (u_char *buff_ptr, u_int32_t *buff_len, 
                 u_int32_t *offset, u_char *val, 
                 u_int32_t val_size)
{
  int i = 0;

  while (i < val_size)
    {
      buff_ptr[*offset] = val[i];
      (*offset)++;
      i++;
    }

  *buff_len -= val_size;
}

static inline void
gvrp_get_octets (u_char *buff_ptr, u_int32_t *buff_len, 
                 u_int32_t *offset, u_char *val, 
                 u_int32_t val_size)
{
  int i = 0;

  while (i < val_size)
    val[i++] = buff_ptr[(*offset)++];

  *buff_len -= val_size;
}

static inline void
gvrp_build_attribute (struct gvrp *gvrp, struct gvrp_port *port, u_char *buff_ptr,
                      u_int32_t *buff_len, u_int32_t *offset,
                      u_char attr_type, u_int32_t gid_index, 
                      enum gid_event event)
{
  u_char attr_event;
  u_char attr_len;
  u_char vid[2];
  u_int16_t vid_short;

  
  vid_short = gvd_get_vid (gvrp->gvd, gid_index);

  vid[0] = (vid_short >> 8) & 0xff;
  vid[1]  = vid_short & 0xff;

  /* Map the received event to the attribute event */
  attr_event = garp_map_gid_event_to_attribute_event (event);

  /* Increment the event counter */
  gvrp->garp_instance.transmit_counters[attr_event]++;
  gvrp->garp_instance.transmit_counters[TOTAL_PACKET_COUNT]++;

  if (port)
    {
      port->transmit_counters[attr_event]++;
    }

  if (attr_event == GARP_ATTR_EVENT_LEAVE_ALL)
    {
      /* Length is atleast leave all attribute size */
      attr_len = GVRP_LEAVE_ALL_SIZE;
    }
  /* all but leave_all events */ 
  else
    {
      /* Length is atleast non leave all size (4 octets) */
      attr_len = GVRP_NON_LEAVE_ALL_SIZE;
    }
  
  /* Set the attribute length */
  gvrp_set_octets (buff_ptr, buff_len, offset, &attr_len, 
                   GARP_ATTRIBUTE_LENGTH_SIZE);

  /* Set the attribute event */
  gvrp_set_octets (buff_ptr, buff_len, offset, &attr_event, 
                   GARP_ATTRIBUTE_EVENT_SIZE);

  /* Set the attribute value */
  if (attr_event != GARP_ATTR_EVENT_LEAVE_ALL)
    {
      /* non leave all contains attribute length, event, value (vid)
         leave all contains attribute length, event only */

      if (GVRP_DEBUG(PACKET))
              zlog_debug (gvrpm, "gvrp_build_attribute: attr_type %u  attr_event "
                        "%u attr_len %u  vid %u", attr_type, attr_event, 
                        attr_len, vid_short);

      gvrp_set_octets (buff_ptr, buff_len, offset, vid, 
                       GVRP_ATTRIBUTE_VALUE_VLAN_SIZE);
    }
}


#ifdef HAVE_MVRP

static inline char
mvrp_build_attribute (struct gvrp *gvrp, struct gvrp_port *port,
                      u_char *buff_ptr, u_int32_t *buff_len, u_int32_t *offset,
                      u_char attr_type, u_int32_t gid_index,
                      enum gid_event event, int *vector)
{
  u_char attr_event;
  u_char vid[2];
  u_int16_t vid_short;

  vid_short = gvd_get_vid (gvrp->gvd, gid_index);

  vid[0] = (vid_short >> 8) & 0xff;
  vid[1] = vid_short & 0xff;

  /* Map the received event to the attribute event */
  if ((port != NULL) && (port->gid_port != NULL)
       && (port->gid_port->p2p == PAL_TRUE))
    attr_event = mrp_map_gid_event_to_p2p_attribute_event (event);
  else
    attr_event = mrp_map_gid_event_to_attribute_event (event);

  /* Increment the event counter */
  gvrp->garp_instance.transmit_counters[attr_event]++;
  gvrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_MAX]++;

  if (port)
    {
      port->transmit_counters[attr_event]++;
    }


  if (GVRP_DEBUG(PACKET))
    zlog_debug (gvrpm, "gvrp_build_attribute: attr_type %u  attr_event "
                "%u vid %u", attr_type, attr_event, vid_short);

  gvrp_set_octets (buff_ptr, buff_len, offset, vid,
                   GVRP_ATTRIBUTE_VALUE_VLAN_SIZE);

  gvrp->encoded_vid = vid_short;

  return attr_event;
}


#endif /* HAVE_MVRP */


int
gvrp_vlan_exist (struct gvrp *gvrp, u_int32_t gid_index)
{
  u_int16_t vid;
  struct nsm_vlan nvlan;
  struct avl_node *rn = NULL;
  struct avl_tree *vlan_table;

  vid = gvd_get_vid (gvrp->gvd, gid_index);
  NSM_VLAN_SET (&nvlan, vid);

  if (NSM_BRIDGE_TYPE_PROVIDER (gvrp->bridge))
    vlan_table = gvrp->bridge->svlan_table;
  else
    vlan_table = gvrp->bridge->vlan_table;

  if (vlan_table == NULL)
    return PAL_FALSE;

  rn = avl_search (vlan_table, (void *)&nvlan);

  if (rn)
      return PAL_TRUE;

  return PAL_FALSE;
}

static void
gvrp_gid_delete_entries (struct gvrp_gvd *gvd, struct gid *gid)
{
  struct ptree_node *rn;
  struct gvrp_attr_entry_tbd *attr_entry_tbd;

  if (!gvd || !gvd->to_be_deleted_attr_table || !gid)
    return;

  for (rn = ptree_top (gvd->to_be_deleted_attr_table); rn;
       rn = ptree_next (rn))
    {
      attr_entry_tbd = rn->info;

      if (!attr_entry_tbd)
        continue;

      if (attr_entry_tbd->pending_gid == 0)
        continue;

      if (gid_delete_attribute (gid, attr_entry_tbd->attribute_index))
        attr_entry_tbd->pending_gid--;
    }
  return;
}

static void
gvrp_gvd_delete_entries (struct gvrp_gvd *gvd, struct gid *gid)
{
  struct ptree_node *rn;
  struct gvrp_attr_entry_tbd *attr_entry_tbd;

  if (!gvd || !gvd->to_be_deleted_attr_table || !gid)
    return;

  for (rn = ptree_top (gvd->to_be_deleted_attr_table); rn;
       rn = ptree_next (rn))
    {
      attr_entry_tbd = rn->info;

      if (!attr_entry_tbd)
        continue;

      if (attr_entry_tbd->pending_gid > 0)
        continue;

      if (gid_app_object_is_to_be_deleted (gid, attr_entry_tbd->attribute_index))
        {
          gip_delete_attribute (gid, attr_entry_tbd->attribute_index);

          gvd_delete_entry (gvd, attr_entry_tbd->attribute_index);

          /* Delete the entry in to_be_deleted_attr_table */
          PREFIX_ATTRIBUTE_INFO_UNSET (rn);
          XFREE (MTYPE_GMRP_GMD_ENTRY, attr_entry_tbd);
        }
    }
  return;
}

static inline bool_t
gvrp_build_message (struct gvrp *gvrp, struct gvrp_port *port, struct gid *gid,
                    u_char *buff_ptr, u_int32_t *buff_len, 
                    u_int32_t *offset, u_int32_t *ret_gid_index)
{
  u_char attr_type;
  u_char event;
  u_int32_t gid_index;
  u_int32_t attr_index = 0;
  u_int32_t start_index = 0;
  struct gid_port *gid_port = NULL;
  u_char leave_all_flag = 0;

  if ((!gid) || (!(gid_port = gid->gid_port)) || (!port))
    return attr_index;

  if ((event = gid_tx_attribute (gid, start_index, &gid_index)) != 
      GID_EVENT_NULL)
    {
      if (event == GID_EVENT_TX_LEAVE_ALL)
        leave_all_flag = 1;

      /* Find the attribute type */ 
      attr_type = GVRP_ATTRIBUTE_TYPE_VLAN;

      /* Set the attribute end mark */
      gvrp_set_octets (buff_ptr, buff_len, offset, &attr_type,
                       GARP_ATTRIBUTE_TYPE_SIZE);

      gvrp_build_attribute (gvrp, port, buff_ptr, buff_len, offset,
                            attr_type, gid_index, event);

      attr_index++;
      
      while ( (*buff_len > GVRP_NON_LEAVE_ALL_SIZE) &&
              (attr_index < GVRP_MAX_ATTR_PER_PDU) &&
              ((event = gid_tx_attribute (gid, start_index, &gid_index))
                                          != GID_EVENT_NULL))
        {
          if (leave_all_flag)
            {
              if ( (event == GID_EVENT_TX_EMPTY) &&
                   (!gvrp_vlan_exist (gvrp, gid_index)))
                continue;
            }

          gvrp_build_attribute (gvrp, port, buff_ptr, buff_len, offset,
                                attr_type, gid_index, event);
          attr_index++;
        }
    }

  if (attr_index)
    {
      /* Set the End Mark Attribute */
      u_char end_mark = GARP_ATTRIBUTE_END_MARK;
      gvrp_set_octets (buff_ptr, buff_len, offset, &end_mark,
                       GARP_ATTRIBUTE_END_MARK_SIZE);

      if (GVRP_DEBUG(PACKET))
        {
          zlog_debug (gvrpm, "gvrp_build_message: [port %d] Number of "
                      "attributes in the message %u", gid_port->ifindex,
                      attr_index);
        }
    }

  *ret_gid_index = gid_index;
  
  /* Check for any attributes to be deleted and delete those */
  gvrp_gid_delete_entries (gvrp->gvd, gid);
  gvrp_gvd_delete_entries (gvrp->gvd, gid);

  return attr_index;
}

#ifdef HAVE_MVRP

static inline bool_t
mvrp_build_message (struct gvrp *gvrp, struct gvrp_port *port,
                    struct gid *gid, u_char *buff_ptr, u_int32_t *buff_len,
                    u_int32_t *offset, u_int32_t *ret_gid_index)
{
  u_char event;
  u_int16_t vid;
  int vector = 0;
  u_char attr_type;
  u_char attr_event;
  u_int32_t gid_index;
  u_int8_t p2p_vector = 0;
  u_int32_t attr_index = 0;
  u_int32_t start_index = 0;
  u_int16_t no_of_values = 0;
  struct gid_port *gid_port = NULL;
  u_char vector_octet [MVRP_VECTOR_SIZE];
  u_int8_t tx_event = MRP_TX_EVENT_NULL;
  u_char no_of_val_octet [MVRP_NUM_OF_VALUES_SIZE];
  u_int8_t leave_all_event = MRP_LEAVE_ALL_EVENT_NULL;
  u_char end_mark;

  if ((!gid) || (!(gid_port = gid->gid_port)))
   return attr_index;

  mad_get_tx_leave_all_event (gid, &tx_event, &leave_all_event);

  /* Find the attribute type */
  attr_type = GVRP_ATTRIBUTE_TYPE_VLAN;

  /* Set the attribute type */
  gvrp_set_octets (buff_ptr, buff_len, offset, &attr_type,
                   GARP_ATTRIBUTE_TYPE_SIZE);

  while (((event = mad_tx_attribute (gid, start_index, &gid_index, 
                                     tx_event)) != GID_EVENT_NULL))
    {
      if (*offset >= GVRP_MAX_PDU_OFFSET) 
          break;
      if (gid_port->p2p == PAL_TRUE)
        no_of_values = MRP_NUM_PACKED_EVENT_P2P;
      else
        no_of_values = MRP_NUM_PACKED_EVENT_NOT_P2P;

      no_of_values = no_of_values +
                     leave_all_event * (MRP_NUM_ATTR_EVENT + 1);

      mrp_no_of_val_to_octets (&no_of_values, no_of_val_octet);

      gvrp_set_octets (buff_ptr, buff_len, offset,
                       no_of_val_octet, MVRP_NUM_OF_VALUES_SIZE);

      vector = 0;

      vid = gvd_get_vid (gvrp->gvd, gid_index);          

      attr_event = mvrp_build_attribute (gvrp, port, buff_ptr, buff_len,
                                         offset, attr_type, gid_index,
                                         event, &vector);
      if (gid_port->p2p == PAL_TRUE)
        {
          vector = (vector * MRP_VEC_MUL_FAC_P2P) + attr_event;
          mrp_add_null_events (&vector, 1,
                               MRP_NUM_PACKED_EVENT_P2P,
                               MRP_VEC_MUL_FAC_P2P);
          if (port != NULL)
            {
              port->transmit_counters[MRP_ATTR_EVENT_NULL]
              += MRP_NUM_PACKED_EVENT_P2P - 1;
              port->transmit_counters[MRP_ATTR_EVENT_MAX]
              += MRP_NUM_PACKED_EVENT_P2P - 1;
            }

          gvrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_NULL]
             += MRP_NUM_PACKED_EVENT_P2P - 1;
          gvrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_MAX]
             += MRP_NUM_PACKED_EVENT_P2P - 1;
        }
      else
        {
          vector = (vector * MRP_VEC_MUL_FAC_NOT_P2P) + attr_event;
          mrp_add_null_events (&vector, 1,
                               MRP_NUM_PACKED_EVENT_NOT_P2P,
                               MRP_VEC_MUL_FAC_NOT_P2P);

          if (port != NULL)
            {
              port->transmit_counters[MRP_ATTR_EVENT_NULL]
              += MRP_NUM_PACKED_EVENT_NOT_P2P - 1;
              port->transmit_counters[MRP_ATTR_EVENT_MAX]
              += MRP_NUM_PACKED_EVENT_NOT_P2P - 1;
            }

          gvrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_NULL]
             += MRP_NUM_PACKED_EVENT_NOT_P2P - 1;
          gvrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_MAX]
             += MRP_NUM_PACKED_EVENT_NOT_P2P - 1;
        }

      if (gid_port->p2p == PAL_TRUE)
        {
           p2p_vector = vector & 0xFF;

           gvrp_set_octets (buff_ptr, buff_len, offset, &p2p_vector,
                            MRP_COMPACT_VECTOR_SIZE);
        }
      else
        {
           mrp_vector_to_octets (&vector, vector_octet);

           gvrp_set_octets (buff_ptr, buff_len, offset, vector_octet,
                            MRP_VECTOR_SIZE);
        }
    }

  /* Set the attribute end mark */
  end_mark = GARP_ATTRIBUTE_END_MARK;

  gvrp_set_octets (buff_ptr, buff_len, offset, &end_mark,
                   GARP_ATTRIBUTE_END_MARK_SIZE);

  /* Set the End Mark Attribute */
  gvrp_set_octets (buff_ptr, buff_len, offset, &end_mark,
                   GARP_ATTRIBUTE_END_MARK_SIZE);

  if (GVRP_DEBUG(PACKET))
    {
      zlog_debug (gvrpm, "gvrp_build_message: [port %d] Number of "
                  "attributes in the message %u", gid_port->ifindex, 
                  attr_index);
    }

  attr_index++;

  *ret_gid_index = gid_index;

  /* Check for any attributes to be deleted and delete those */
  gvrp_gid_delete_entries (gvrp->gvd, gid);
  gvrp_gvd_delete_entries (gvrp->gvd, gid);

  return attr_index;
}

#endif /* HAVE_MVRP */

static inline bool_t
gvrp_build_pdu (struct gvrp *gvrp, struct gvrp_port *port, struct gid *gid, 
                u_char *buff_ptr, u_int32_t *buff_len,
                u_int32_t *gid_index)
{
  u_char llc_format[3] = {0x42, 0x42, 0x03};
  u_char protocolID[GARP_PROTOCOL_ID_SIZE];
  struct gid_port *gid_port = NULL;
  u_int32_t message_index = 0;
  u_int32_t offset = 0;
  u_char min_len;

  if ((!gid) || (!(gid_port = gid->gid_port)) || (!port))
    return message_index;

#ifdef HAVE_MVRP
  if (gvrp->reg_type == REG_TYPE_MVRP)
    {
      min_len = (GARP_ATTRIBUTE_TYPE_SIZE + GARP_ATTRIBUTE_LENGTH_SIZE
                 + GVRP_NON_LEAVE_ALL_SIZE
                 + GARP_ATTRIBUTE_END_MARK_SIZE + GARP_PDU_END_MARK);
   }
 else
#endif /* HAVE_MVRP*/
   {
      min_len = (GARP_NO_OF_VALUES_SIZE + GVRP_NON_LEAVE_ALL_SIZE +
                 GARP_VECTOR_SIZE + GARP_ATTRIBUTE_END_MARK_SIZE +
                 GARP_PDU_END_MARK);
   }

#ifdef HAVE_MVRP
  if (gvrp->reg_type == REG_TYPE_MVRP)
    {
      protocolID[0] = 0x00;
      gvrp_set_octets (buff_ptr, buff_len, &offset, protocolID,
                       MRP_PROTOCOL_ID_SIZE);
    }
  else
#endif /* HAVE_MVRP */
    {
      
      /* Set the LLC encapsulation */
      gvrp_set_octets (buff_ptr, buff_len, &offset, llc_format, 3);

      /* Set the Protocol ID */
      protocolID[0] = 0x00;
      protocolID[1] = 0x01;

      gvrp_set_octets (buff_ptr, buff_len, &offset, protocolID, 
                       GARP_PROTOCOL_ID_SIZE);
    }

#ifdef HAVE_MVRP
  if (gvrp->reg_type == REG_TYPE_MVRP)
    {
      if ( mvrp_build_message (gvrp, port, gid, buff_ptr, buff_len, &offset,
                               gid_index))
        message_index++;
    }
  else
#endif /* HAVE_MVRP */
    {
      if (gvrp_build_message (gvrp, port, gid, buff_ptr, buff_len, &offset,
                              gid_index))
        message_index++;
    }

  /* Set the End Mark Pdu */
  if (message_index)
    {
      u_char end_mark = GARP_PDU_END_MARK;
      gvrp_set_octets (buff_ptr, buff_len, &offset, &end_mark, 
                       GARP_PDU_END_MARK_SIZE);

      if (GVRP_DEBUG(PACKET))
        zlog_debug (gvrpm, "gvrp_build_pdu: [port %d] Number of messages "
                    "in the pdu %u", gid_port->ifindex, message_index);
    }

  return message_index;
}

static inline bool_t 
gvrp_parse_attribute (struct gvrp *gvrp, struct gvrp_port *port,
                      struct gid *gid,
                      u_char *buff_ptr, u_int32_t *buff_len, 
                      u_int32_t *offset, u_char attr_type)
{
  struct nsm_if *zif = NULL;
  struct nsm_vlan_port *vlan_port = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct interface *ifp = NULL; 
  enum gid_event event;
  u_int32_t gid_index;
  u_int32_t starting_gid_index;
  u_char octet;
  u_char vid[2]; 
  u_int16_t temp_vid;
  struct gid_port *gid_port = NULL;

  if ((!gid) || (!(gid_port = gid->gid_port)) )
    return PAL_FALSE;

  if ( ! port )
    return PAL_FALSE;

  ifp = (struct interface *)port->port;
  if ( ! ifp )
    return PAL_FALSE;

  zif = (struct nsm_if *)ifp->info;
  if ( ! zif )
    return PAL_FALSE;

  br_port = zif->switchport;
  if (! br_port )
    return PAL_FALSE;

  vlan_port = &(br_port->vlan_port);
  if ( ! vlan_port )
    return PAL_FALSE;

  /* Intialize to a dummy value */
  gid_index = GVRP_NUMBER_OF_GID_MACHINES;

  /* Get the attribute length */
  gvrp_get_octets (buff_ptr, buff_len, offset, &octet, 
                   GARP_ATTRIBUTE_LENGTH_SIZE);

  /* Check if the attribute length is between min and max buff_len.
     Eventhough the max buff_len is not checked any value greater than
     max will cause an overflow and thus will result lesser than the
     min */
  if (octet < GARP_ATTRIBUTE_LENGTH_MIN)
    return PAL_FALSE; 

  /* Get the attribute event */
  gvrp_get_octets (buff_ptr, buff_len, offset, &octet, 
                   GARP_ATTRIBUTE_EVENT_SIZE);

  /* Check if the event value is is lesser than GARP_ATTR_EVENT_MAX
   * If the it is an invalid event skip the attribute value.
   * Since we have an invalid event assume that the event is not
   * leave all and we extract the attribute value.
   */

  if (octet > GARP_ATTR_EVENT_MAX)
    {
      if (*buff_len < GVRP_ATTRIBUTE_VALUE_VLAN_SIZE)
        {
          *buff_len = 0;
          return PAL_FALSE;
        }
      gvrp_get_octets (buff_ptr, buff_len, offset, vid, 
                       GVRP_ATTRIBUTE_VALUE_VLAN_SIZE);
      return PAL_TRUE;
    }

  /* Convert the event received to actual gid events */
  event = garp_map_attribute_event_to_gid_event (octet);

  /* Incrememt the counter */
  gvrp->garp_instance.receive_counters[octet]++;
  port->receive_counters[octet]++;

  if (event == GID_EVENT_RCV_LEAVE_ALL)
    { 
      starting_gid_index = 0;
      gid_rcv_leave_all (gid, starting_gid_index, 0);
      
      return PAL_TRUE;     
    }

  /* Get the attribute value  */
  gvrp_get_octets (buff_ptr, buff_len, offset, vid, 
                   GVRP_ATTRIBUTE_VALUE_VLAN_SIZE);

  temp_vid = pal_ntoh16 (*(u_int16_t *)(vid));

  /* if the vlan id value is invalid discard it */
  if ( (temp_vid < NSM_VLAN_DEFAULT_VID) || (temp_vid > NSM_VLAN_MAX) )
    return PAL_FALSE;

  /* Get the gid_index associated with the mac address from the GVD */
  if (! gvd_find_entry (gvrp->gvd, temp_vid, &gid_index))
    {
      if (!gvd_create_entry (gvrp->gvd, temp_vid, &gid_index))
              {
                zlog_debug (gvrpm, "gvrp_parse_attribute: [port %d]"
                            "ERROR: DATABASE FULL", gid_port->ifindex);
              }
    }

  /* If a join-in is received for a vlan that is already configured
   * on the port, then the GID must not be entered in the
   * Registrar State Machine.
   */
  if (!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, temp_vid)))
    {
      /* Call the GARP layer to handle the port, attribute, event */
      gid_rcv_attribute (gid, gid_index, event);
    }

  return PAL_TRUE;
}

#ifdef HAVE_MVRP

static inline bool_t
mvrp_parse_attribute (struct gvrp *gvrp, struct gvrp_port *port,
                      struct gid *gid, u_char *buff_ptr, u_int32_t *buff_len,
                      u_int32_t *offset, u_char attr_type)
{
  u_char vid[2];
  u_int32_t attr;
  u_char octet[4];
  u_int16_t index;
  u_int32_t vector;
  u_int16_t temp_vid;
  u_int32_t gid_index;
  enum gid_event event;
  u_int16_t no_of_values;
  u_int32_t no_of_vector;
  s_int32_t increment = 0;
  struct nsm_if *zif = NULL;
  u_int32_t starting_gid_index;
  struct interface *ifp = NULL;
  struct gid_port *gid_port = NULL;
  struct nsm_bridge_port *br_port= NULL;
  struct nsm_vlan_port *vlan_port = NULL;

  if ( ! port )
    return PAL_FALSE;

  ifp = (struct interface *)port->port;
  if ( ! ifp )
    return PAL_FALSE;

  zif = (struct nsm_if *)ifp->info;
  if ( ! zif )
    return PAL_FALSE;

  br_port = zif->switchport;
  if (! br_port )
    return PAL_FALSE;

  vlan_port = &(br_port->vlan_port);
  if ( ! vlan_port )
    return PAL_FALSE;


  if ((!gid) || (!(gid_port = gid->gid_port)))
   return PAL_FALSE;

/* Get the attribute buff_len */
  gvrp_get_octets (buff_ptr, buff_len, offset, octet,
                   GARP_NO_OF_VALUES_SIZE);

  no_of_values = (octet[0] << 8);
  no_of_values |= octet[1];

  if ((attr = MRP_DECODE_LEAVE_ALL_VAL))
    {
      event = mrp_map_leave_all_event_to_gid_event (attr);
      starting_gid_index = 0;
      gid_rcv_leave_all (gid, starting_gid_index, 0);

    }

   no_of_values = no_of_values % (MRP_NUM_ATTR_EVENT + 1);
      
   if (gid_port->p2p)
    no_of_vector = no_of_values / MRP_NUM_PACKED_EVENT_P2P;
   else
    no_of_vector = no_of_values / MRP_NUM_PACKED_EVENT_NOT_P2P;

 
    if (no_of_vector == 0)
      return PAL_FALSE;

    for (index = 0; index < no_of_vector; index++)
       {
         gvrp_get_octets (buff_ptr, buff_len, offset, vid,
                          GVRP_ATTRIBUTE_VALUE_VLAN_SIZE);

         /* Get the attribute event */
         if (gid_port->p2p == PAL_TRUE)
           {
             gvrp_get_octets (buff_ptr, buff_len, offset, octet,
                              MRP_COMPACT_VECTOR_SIZE);
             MVRP_DECODE_P2P_VECTOR;

             increment = MRP_NUM_PACKED_EVENT_P2P - 1;
           }
         else
           {
             gvrp_get_octets (buff_ptr, buff_len, offset, octet,
                              MRP_VECTOR_SIZE);
             MVRP_DECODE_NON_P2P_VECTOR;
             increment = MRP_NUM_PACKED_EVENT_NOT_P2P - 1;
           }

         while (increment >= 0)
           {
             if (gid_port->p2p)
               attr = vector % MRP_VEC_MUL_FAC_P2P;
             else
               attr = vector % MRP_VEC_MUL_FAC_NOT_P2P;

             gvrp->garp_instance.receive_counters[attr]++;
             port->receive_counters[attr]++;

             /* Convert the event received to actual gid events */
             event = mrp_map_attribute_event_to_gid_event (attr);
 
             temp_vid = pal_ntoh16 (*(u_int16_t *)(vid));

             temp_vid += increment;

             /* if the vlan id value is invalid discard it */
             if (( temp_vid < NSM_VLAN_DEFAULT_VID) || 
                  (temp_vid > NSM_VLAN_MAX) )
               return PAL_FALSE;

             /* Get the gid_index associated with the mac address 
              from the GVD */
             if (event != GID_EVENT_NULL)
               {
                 if (! gvd_find_entry (gvrp->gvd, temp_vid, &gid_index))
                   {
                     if (!gvd_create_entry (gvrp->gvd, temp_vid, &gid_index))
                       zlog_debug (gvrpm, "gvrp_parse_attribute: [port %d]"
                                   "ERROR: DATABASE FULL", gid_port->ifindex);
                   }

                 if (!(NSM_VLAN_BMP_IS_MEMBER (vlan_port->staticMemberBmp, 
                                               temp_vid)))
                   {
                     /* Call the GARP layer to handle the port, attribute, 
                        event */
                     gid_rcv_attribute (gid, gid_index, event);
                   }
               }

             vector -= attr;
             increment--;

             if (gid_port->p2p)
               vector = (vector / MRP_VEC_MUL_FAC_P2P);
             else
               vector = (vector / MRP_VEC_MUL_FAC_NOT_P2P);
           }
       }

  return PAL_TRUE;
}

#endif /* HAVE_MVRP */

static void
gvrp_skip_attribute ( u_char *buff_ptr, u_int32_t *buff_len,
                       u_int32_t *offset)
{
  u_char octet;
  u_char vid [GVRP_ATTRIBUTE_VALUE_VLAN_SIZE];
  enum gid_event event = GID_EVENT_MAX; 

  /* In this procedure it is made sure that the buff_len is reset to
   * 0 before returning if the message is not according to
   * GVRP packet format. This is to ensure that gvrp_parse_message
   * loop terminates in case of corrupted message.
   */

  if (*buff_len < GARP_ATTRIBUTE_LENGTH_SIZE)
    {
      *buff_len = 0;
      return;
    }

  gvrp_get_octets (buff_ptr, buff_len, offset, &octet,
                   GARP_ATTRIBUTE_LENGTH_SIZE);

  if (*buff_len < GARP_ATTRIBUTE_EVENT_SIZE)
    {
      *buff_len = 0;
      return;
    } 

  gvrp_get_octets (buff_ptr, buff_len, offset, &octet,
                   GARP_ATTRIBUTE_EVENT_SIZE);

  /* Check if the event value is is lesser than GARP_ATTR_EVENT_MAX
   * If the it is an invalid event skip the attribute value.
   * Since we have an invalid event assume that the event is not
   * leave all and we extract the attribute value.
   */  
  if (octet > GARP_ATTR_EVENT_MAX)
    {
      if (*buff_len < GVRP_ATTRIBUTE_VALUE_VLAN_SIZE)
        {
          *buff_len = 0;
          return;
        }
      gvrp_get_octets (buff_ptr, buff_len, offset, vid,
                       GVRP_ATTRIBUTE_VALUE_VLAN_SIZE);
      return;
    }

  /* Convert the event received to actual gid events */
  event = garp_map_attribute_event_to_gid_event (octet);

  if (event == GID_EVENT_RCV_LEAVE_ALL)
    {
      return;
    }

  if (*buff_len < GVRP_ATTRIBUTE_VALUE_VLAN_SIZE)
    {
      *buff_len = 0;
      return;
    }

  gvrp_get_octets (buff_ptr, buff_len, offset, vid,
                   GVRP_ATTRIBUTE_VALUE_VLAN_SIZE);
  return;

}
                     
static void
gvrp_skip_message ( u_char *buff_ptr, u_int32_t *buff_len,
                    u_int32_t *offset)
{
  u_char octet;

  while (buff_len)
    {
      gvrp_skip_attribute (buff_ptr, buff_len, offset);

      if (*buff_len < GARP_ATTRIBUTE_LENGTH_SIZE)
        {
          *buff_len = 0;
           return;
        }
      /* Get a single octet and check if we reached the end mark */
      gvrp_get_octets (buff_ptr, buff_len, offset, &octet,
                       GARP_ATTRIBUTE_LENGTH_SIZE);
      if (octet == GARP_ATTRIBUTE_END_MARK) 
        {
          return;
        }
      else
        { 
          /* If we have not reached the end mark yet restore the buffer to
           * original offset.
           */
          *buff_len += GARP_ATTRIBUTE_LENGTH_SIZE;
          *offset   -= GARP_ATTRIBUTE_LENGTH_SIZE;
        }
    }
}

static inline bool_t 
gvrp_parse_message (struct gvrp *gvrp, struct gvrp_port *port,
                    struct gid *gid, 
                    u_char *buff_ptr, u_int32_t *buff_len, 
                    u_int32_t *offset)
{
  u_char octet;
  u_char attr_type; 
  u_int32_t attr_index = 0;
  struct gid_port *gid_port = NULL;

  if ((!gid) || (!(gid_port = gid->gid_port)) )
    return PAL_FALSE;
  
  /* Get the attribute type */
  gvrp_get_octets (buff_ptr, buff_len, offset, &octet, 
                   GARP_ATTRIBUTE_TYPE_SIZE);
  attr_type = octet; 

  /* Check if the attribute type is between 1 and 255 only */
  if (attr_type != GVRP_ATTRIBUTE_TYPE_VLAN)
    {
      gvrp_skip_message (buff_ptr, buff_len, offset);
      zlog_debug (gvrpm, "gvrp_parse_message: [port %d] Invalid attribute type ",
                  gid_port->ifindex);
      return PAL_TRUE;
    }

  while (*buff_len)
    {
#ifdef HAVE_MVRP
     if (gvrp->reg_type == REG_TYPE_MVRP)
       {
         if (! mvrp_parse_attribute (gvrp, port, gid, buff_ptr,
                                     buff_len, offset, attr_type))
           {
            /* Improper/invalid formatted attribute */
            if (GVRP_DEBUG(PACKET))
              zlog_debug (gvrpm, "gvrp_parse_message: [port %d] Error while"
                          "parsing attribute type %u num attributes parsed %u",
                           gid_port->ifindex, attr_type, attr_index);
              return PAL_FALSE;
           }
       }
     else
#endif /* HAVE_MVRP */
       {
        if (gvrp->reg_type == REG_TYPE_GVRP)
          {
            if (! gvrp_parse_attribute (gvrp, port, gid, buff_ptr,
                                        buff_len, offset, attr_type))
              {
                /* Improper/invalid formatted attribute */
                if (GVRP_DEBUG(PACKET))
                  zlog_debug (gvrpm, "gvrp_parse_message: [port %d] Error while"
                       "parsing attribute type %u num attributes parsed %u",
                       gid_port->ifindex, attr_type, attr_index);

                return PAL_FALSE;
              }
           }
       }
       
      attr_index++;

      /* Get a single octet and check if we reached the end mark */
      gvrp_get_octets (buff_ptr, buff_len, offset, &octet, 
                       GARP_ATTRIBUTE_LENGTH_SIZE);

#ifdef HAVE_MVRP
     if (gvrp->reg_type == REG_TYPE_MVRP)
       {
         if (octet == GARP_ATTRIBUTE_END_MARK)
           {
             /* Get one more octet and check if we reached the end mark */
             gvrp_get_octets (buff_ptr, buff_len, offset, &octet,
                              GARP_ATTRIBUTE_LENGTH_SIZE);

             if (octet == GARP_ATTRIBUTE_END_MARK)
               {
                 if (attr_index)
                   {
                     /* Atleast one attribute in the message */
                     if (GVRP_DEBUG(PACKET))
                       zlog_debug (gvrpm, "gvrp_parse_message:[port %d] End of "
                                   "pdu attribute type %u num attributes %u",
                                   gid_port->ifindex, attr_type, attr_index);
                     break;
                   }
                 else
                   {
                     /* No attribute in the message */
                     if (GVRP_DEBUG(PACKET))
                       zlog_debug (gvrpm, "gvrp_parse_message: [port %d] Error "
                                   "No attribute found, end of pdu",
                                   gid_port->ifindex);

                     return PAL_FALSE;
                   }
               }
             else
               {
                 /* If we have not reached the end mark yet restore the buffer
                  * to original offset.
                  */
                 *buff_len += 2 * GARP_ATTRIBUTE_LENGTH_SIZE;
                 *offset   -= 2 * GARP_ATTRIBUTE_LENGTH_SIZE;
               }
           }
         else
           {
             /* If we have not reached the end mark yet restore the buffer to
              * original offset.
              */
             *buff_len += GARP_ATTRIBUTE_LENGTH_SIZE;
             *offset   -= GARP_ATTRIBUTE_LENGTH_SIZE;
           }
       }
     else
#endif /* HAVE_MVRP */
       {
         if (octet == GARP_ATTRIBUTE_END_MARK)
           {
             if (attr_index)
               {
                 /* Atleast one attribute in the message */
                 if (GVRP_DEBUG(PACKET))
                   zlog_debug (gvrpm, "gvrp_parse_message:[port %d] End of pdu "
                               "attribute type %u num attributes %u",
                                gid_port->ifindex, attr_type, attr_index);
                 break;
               }
             else
               {
                 /* No attribute in the message */
                 if (GVRP_DEBUG(PACKET))
                   zlog_debug (gvrpm, "gvrp_parse_message: [port %d] Error "
                               "No attribute found, end of pdu",
                               gid_port->ifindex);

                 return PAL_FALSE;
               }
           }
         else
           {
             /* If we have not reached the end mark yet restore the buffer to
              * original offset.
              */
             *buff_len += GARP_ATTRIBUTE_LENGTH_SIZE;
             *offset   -= GARP_ATTRIBUTE_LENGTH_SIZE;
           }
       }
    }

  return PAL_TRUE;
}

void
gvrp_parse_pdu (struct gvrp *gvrp, struct gvrp_port *port,
                struct gid *gid, u_char *buff_ptr, u_int32_t buff_len)
{
  u_char octet;
  u_char protocolID[2];
  u_int32_t msg_index = 0;
  u_int32_t offset = 0;
  struct gid_port *gid_port = NULL;

  if ((!gid) || (!(gid_port = gid->gid_port)) )
    return;

#ifdef HAVE_MVRP

  if (gvrp->reg_type == REG_TYPE_MVRP)
    {
      gvrp_get_octets (buff_ptr, &buff_len, &offset, protocolID,
                       MRP_PROTOCOL_ID_SIZE);

      if (protocolID [0] != MRP_PROTOCOL_ID)
        {
          if (GVRP_DEBUG(PACKET))
            zlog_debug (gvrpm, "gvrp_parse_pdu: [port %d] Invalid protocolID %u "
                        "received on the pdu", gid_port->ifindex, protocolID);
          return;
        }
    }
  else
#endif /* HAVE_MVRP */
    {
      /* Get the protocolID */
      gvrp_get_octets (buff_ptr, &buff_len, &offset, protocolID, 
                       GARP_PROTOCOL_ID_SIZE);

      /* Check if the protocolID is valid */
      if (pal_ntoh16(((u_int16_t *)protocolID)[0]) != GARP_PROTOCOL_ID)
        {
          if (GVRP_DEBUG(PACKET))
            zlog_debug (gvrpm, "gvrp_parse_pdu: [port %d] Invalid protocolID %u "
                        "received on the pdu", gid_port->ifindex, protocolID);
          return;
        }
    }

  /* Sec 12.9.1 Registrar administrative control values */
  /* If in forbidden mode ignore all garp messages */
  if (port->registration_mode == GVRP_VLAN_REGISTRATION_FORBIDDEN)
    return;
  
  while (buff_len)
    {
      if (! gvrp_parse_message (gvrp, port, gid, buff_ptr, &buff_len, &offset))
        {
          /* Improper/invalid formatted message */
          if (GVRP_DEBUG(PACKET))
            zlog_debug (gvrpm, "gvrp_parse_pdu: [port %d] Error while parsing "
                      "message", gid_port->ifindex);
          break;
        }

      msg_index++;

      if (buff_len == 0)
        break;

      /* Get a single octet and check if we reached the end mark */
      gvrp_get_octets (buff_ptr, &buff_len, &offset, &octet, 
                       GARP_ATTRIBUTE_LENGTH_SIZE);

      if (octet == GARP_PDU_END_MARK)
        {
          if (msg_index)
            {
              /* Atleast one message in the pdu */
              if (GVRP_DEBUG(PACKET))
                zlog_debug (gvrpm, "gvrp_parse_pdu: [port %d] End of pdu "
                            "num messages %u", gid_port->ifindex, msg_index);
              break; 
            }
          else
            { /* No message in the pdu */
              if (GVRP_DEBUG(PACKET))
                zlog_debug (gvrpm, "gvrp_parse_message: [port %d] Error no "
                          "message found end of pdu", gid_port->ifindex);
              break;
            }
        }
      else
        {
          /* If we have not reached the end mark yet restore the buffer to
           * original offset.
           */
          buff_len += GARP_ATTRIBUTE_LENGTH_SIZE;
          offset   -= GARP_ATTRIBUTE_LENGTH_SIZE;
        }
    }

  /* Inform gvrp to propagate the information received to all the other ports
     that are part of the bridge and the port state is forwarding */
  gvrp_do_actions (gid);
  
  if (GVRP_DEBUG(PACKET))
    zlog_debug (gvrpm, "gvrp_parse_pdu: [port %d] Num messages in the pdu is "
                "%u", gid_port->ifindex, msg_index);

  return;
}

bool_t
gvrp_transmit_pdu (void *application, struct gid *gid)
{
  /* Build the pdu */
  u_char buff[GVRP_PDU_SIZE];
  u_char *bufptr = buff;
  u_int32_t buff_len, total_len = 0;
  struct gvrp *gvrp;
  struct gvrp_port *port;
  struct nsm_vlan_port *vlan_port;
  struct nsm_bridge_port *br_port = NULL;
  struct interface *ifp;
  struct nsm_if *zif;
  struct apn_vr *vr;
  u_char octet;
  int ret;
  u_int32_t registration_type;
  u_int32_t gid_index = 0;
  struct gid_port *gid_port = NULL;

  if ((!gid) || (!(gid_port = gid->gid_port)) )
    return PAL_FALSE;

  gvrp = (struct gvrp*)application;

  gvrp->gvrp_last_pdu_origin = gid_port->ifindex;

  vr = apn_vr_get_privileged (gvrpm);
  if (vr == NULL)
    return PAL_FALSE;

  ifp = if_lookup_by_index (&vr->ifm, gid_port->ifindex);
  if (ifp == NULL)
    return PAL_FALSE;

  if (GVRP_DEBUG(PACKET))
    zlog_debug (gvrpm, "gvrp_transmit_pdu: [port %d](%s)  SENDING GVRP PDU "
              "TO OTHER PARTICIPANTS ", gid_port->ifindex, ifp->name);
  
  zif = (struct nsm_if *)ifp->info;

  if ( zif == NULL || zif->switchport == NULL)
    return PAL_FALSE;

  br_port = zif->switchport;
   
  if ( !(port = br_port->gvrp_port) ||
       !(vlan_port = &(br_port->vlan_port)) )
    return PAL_FALSE;

  octet = buff[ATTRIB_EVENT_OFFSET];
  
  ret = gvrp_get_registration(gvrp->bridge->master, ifp, 
      &registration_type);

  if (ret < 0)
    {
      if (GVRP_DEBUG(PACKET))
        zlog_debug (gvrpm, "gvrp_transmit_pdu: couldn't get registration");
      return PAL_FALSE;
    }

  if (registration_type == GVRP_VLAN_REGISTRATION_FORBIDDEN)
    {
      if (GVRP_DEBUG(PACKET))
        zlog_debug (gvrpm, "gvrp_transmit_pdu:Port in Forbidden registration");
      return PAL_TRUE;
    }

  while (gid->tx_pending)
    { 
      buff_len = GVRP_PDU_SIZE;
      pal_mem_set (buff, 0, GVRP_PDU_SIZE);
      bufptr = buff;
      total_len = 0;

      if (GVRP_DEBUG(PACKET))
        zlog_debug (gvrpm, "Gid Index gid_index %d \n", gid_index);

      if (! gvrp_build_pdu (gvrp, port, gid, bufptr, &buff_len, &gid_index))
        {
          if (GVRP_DEBUG(PACKET))
            zlog_err (gvrpm, "GVRP build PDU failed \n");
          return PAL_FALSE;
        }

      /* Increment total_len by buff_len. */
      total_len += (GVRP_PDU_SIZE - buff_len);

      garp_send (gvrp->bridge->name, ifp->hw_addr,
                 NSM_BRIDGE_TYPE_PROVIDER (gvrp->bridge) ?
                 pro_gvrp_dest_addr : gvrp_dest_addr,
                 gid_port->ifindex, buff, total_len, NSM_VLAN_NONE);

      if (gvrp->reg_type == REG_TYPE_GVRP)
        {
          gvrp->garp_instance.transmit_counters[0]++;
          port->transmit_counters[0]++;
        }
    }

  return PAL_TRUE;
}

/* GVRP packet receiver.  Called from L2 frame handler.  */
void
gvrp_receive_pdu (unsigned int port_index, u_int8_t *src_mac,
                  u_char *buff, u_int32_t buff_len)
{
  struct gid *gid;
  struct gvrp *gvrp;
  struct nsm_if *zif;
  struct interface *ifp;
  struct nsm_master *nm;
  struct LLC_Frame_s llc;
  u_char *buff_ptr = buff;
  struct nsm_bridge *bridge;
  struct gvrp_port *gvrp_port;
  struct nsm_bridge_port *br_port;
  struct nsm_bridge_master *master;
  struct apn_vr *vr = apn_vr_get_privileged (gvrpm);
  
  ifp = if_lookup_by_index(&vr->ifm, port_index);
  nm = vr->proto;
  master = nsm_bridge_get_master(nm);

  if ( (!ifp) || (!master) )
    return;
  
  if ( !(zif = (struct nsm_if *)ifp->info) )
    return;
  
  if ( !(bridge = zif->bridge) )
    return;
   
  if ( !(br_port = zif->switchport) )
    return;
   
  if ( !(gvrp = bridge->gvrp) )
    {
      if (GVRP_DEBUG(PACKET))
        zlog_debug (gvrpm, "gvrp_receive_pdu: [port %d] GVRP is not "
                    "configured on this bridge", ifp->ifindex);
      return;
    }

  if ( !(gvrp_port = br_port->gvrp_port) )
    {
      if (GVRP_DEBUG(PACKET))
        zlog_debug (gvrpm, "gvrp_receive_pdu: [port %d] GVRP is not "
                    "configured on this port", ifp->ifindex);
      return;
    }

  if (br_port->state != NSM_BRIDGE_PORT_STATE_FORWARDING)
    {
      if (GVRP_DEBUG(PACKET))
        zlog_debug (gvrpm, "gvrp_receive_pdu: [port %d] GVRP is in"
                    "discarding state", ifp->ifindex);
      return;
    }

  gid = gvrp_port->gid;

  /* When we got GVRP packet from non configured port. */
  if (! gid)
    {
      if (GVRP_DEBUG(PACKET))
        zlog_debug (gvrpm, "gvrp_receive_pdu: [port %d] GVRP is not "
                    "configured on this port", ifp->ifindex);
      return;
    }

   if (gvrp->reg_type == REG_TYPE_GVRP)
     {
       if (l2_llcParse (buff_ptr, &llc) == PAL_FALSE)
         return;

       buff_ptr += 3;
       buff_len -= 3;
     }
   
  pal_mem_cpy (gvrp_port->gvrp_last_pdu_origin, src_mac, ETHER_ADDR_LEN);

  /* Parse packet.  */
  gvrp_parse_pdu (gvrp, gvrp_port, gid, buff_ptr, buff_len);
   
  /* Increment the number of received packet counter.  */
  if (gvrp->reg_type == REG_TYPE_GVRP)
    {
      gvrp->garp_instance.receive_counters[0]++;
      gvrp_port->receive_counters[0]++;
    }
}


bool_t
gvrp_check_vid(struct gvrp *gvrp, u_int16_t vid)
{

  if (gvrp->encoded_vid == 4094)
    return 0;

  if ((gvrp->encoded_vid + 1) == vid)
    return 1;

  return 0;
}



void
mvrp_encode_pdu(struct gvrp *gvrp, u_char *buff_ptr, u_int32_t *buff_len,
                u_int32_t *offset, u_int32_t *no_of_values, u_int32_t *vector)
{
  u_char attr;
  u_char octet[4];
  u_int32_t shift;
  u_char vid[2];

  octet[0] = (*no_of_values & 0x000000FF);
  shift = (*no_of_values >> 8);
  octet[1] = (shift & 0x000000FF);


  gvrp_set_octets (buff_ptr, buff_len, offset, octet,
                   GARP_NO_OF_VALUES_SIZE);

  vid[0] = (gvrp->encoded_vid >> 8) & 0xff;
  vid[1]  = gvrp->encoded_vid & 0xff;


  gvrp_set_octets (buff_ptr, buff_len, offset, vid,
                       GVRP_ATTRIBUTE_VALUE_VLAN_SIZE);

  for (attr = 0; attr < 10; attr++)
     *vector = ((*vector + GARP_MAX_ATTR_EVENT) * 7);

  *vector += GARP_MAX_ATTR_EVENT;

  mvrp_vector_to_octets (vector, octet);

  gvrp_set_octets (buff_ptr, buff_len, offset, octet,
                    GARP_VECTOR_SIZE);

  *no_of_values = 0;
  *vector = 0;
}

void
mvrp_append_attribute (struct gvrp *gvrp, u_char *buff_ptr, u_int32_t *buff_len,
                       u_int32_t *offset, enum gid_event event, int *vector)
{
  u_char attr_event;

  attr_event = garp_map_gid_event_to_attribute_event (event);

  *vector += attr_event;
  *vector *= 7;
}


void
mvrp_vector_to_octets (u_int32_t *vector, u_char *octet)
{
  int shifted = *vector;
  int index,shift;

  shift = 1;

  for (index = 3; index >= 0; index--)
    {
      octet[index] = (shifted & 0x000000FF);
      shifted = (*vector >> (shift*8));
      shift++;
    }
}

