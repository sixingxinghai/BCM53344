/* Copyright 2003  All Rights Reserved. */
#include "pal.h"
#include "lib.h"

#include "l2_llc.h"
#include "nsm_interface.h"
#include "nsm_bridge.h"
#include "nsm_vlan.h"

#include "hal_types.h"
#include "avl_tree.h"

#include "garp_pdu.h"
#include "garp_sock.h"
#include "garp_gid.h"

#include "gmrp.h"
#include "gmrp_database.h"
#include "gmrp_debug.h"
#include "gmrp_pdu.h"
#include "gmrp_api.h"


const u_char
gmrp_dest_addr[HAL_HW_LENGTH] = {0x01, 0x80, 0xC2, 0x00, 0x00, 0x20};

static inline void
gmrp_set_octets (u_char *buff_ptr, u_int32_t *buff_len, 
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
  (*buff_len) -= val_size;
}

static inline void
gmrp_get_octets (u_char *buff_ptr, u_int32_t *buff_len, 
                 u_int32_t *offset, u_char *val, 
                 u_int32_t val_size)
{
  int i = 0;

  while (i < val_size)
    {
      val[i++] = buff_ptr[(*offset)++];
    }

  *buff_len -= val_size;
}


static inline enum gmrp_attribute_type_
gmrp_get_attribute_type (u_int32_t gid_index)
{
   return (gid_index >= GMRP_NUMBER_OF_LEGACY_CONTROL) ?
                        GMRP_ATTRIBUTE_TYPE_GROUP_MEMBERSHIP :
                        GMRP_ATTRIBUTE_TYPE_SERVICE_REQUIREMENT;
}


static inline void
gmrp_build_attribute (struct gmrp *gmrp, u_char *buff_ptr,
                      u_int32_t *buff_len, u_int32_t *offset,
                      u_char attr_type, u_int32_t gid_index, 
                      enum gid_event event)
{
  u_char attr_event;
  u_char attr_len;

  /* Map the received event to the attribute event */
  attr_event = garp_map_gid_event_to_attribute_event (event);
  /* Increment the event counter */
  gmrp->garp_instance.transmit_counters[attr_event]++;
  gmrp->garp_instance.transmit_counters[TOTAL_PACKET_COUNT]++;

  if (attr_event == GARP_ATTR_EVENT_LEAVE_ALL)
    { /* Length is atleast leave all attribute size */
      attr_len = GMRP_LEAVE_ALL_SIZE;
    }
  else if (attr_type == GMRP_ATTRIBUTE_TYPE_GROUP_MEMBERSHIP)
    { /* Length is atleast group membership non leave all size */
      attr_len = GMRP_GROUP_MEMBERSHIP_NON_LEAVE_ALL_SIZE;
    }
  else if (attr_type == GMRP_ATTRIBUTE_TYPE_SERVICE_REQUIREMENT)
    { /* Length is atleast service requirement non leaveall size */
      attr_len = GMRP_SERVICE_REQUIREMENT_NON_LEAVE_ALL_SIZE;
    }
  else
    {
      attr_len = GMRP_GROUP_MEMBERSHIP_NON_LEAVE_ALL_SIZE;
    }
  
  /* Set the attribute buff_len */
  gmrp_set_octets (buff_ptr, buff_len, offset, &attr_len, 
                   GARP_ATTRIBUTE_LENGTH_SIZE);

  /* Set the attribute event */
  gmrp_set_octets (buff_ptr, buff_len, offset, &attr_event, 
                   GARP_ATTRIBUTE_EVENT_SIZE);

  /* Return if the event is leave all
   * leave all contains only attribute buff_len and attribute event
   */
  if ( attr_event == GARP_ATTR_EVENT_LEAVE_ALL)
    {
      return;
    }

  if (attr_type == GMRP_ATTRIBUTE_TYPE_GROUP_MEMBERSHIP)
    { /* attribute value = 6 byte Mac address */
      u_char mac_addr[HAL_HW_LENGTH];
      
      /* Get the Multicast Mac Address associated with the GID from the GMD */
      pal_assert (gmd_get_mac_addr (gmrp->gmd, gid_index, mac_addr));

      if (GMRP_DEBUG(PACKET))
        {
          zlog_debug (gmrpm, "gmrp_build_attribute: attr_type %u, attr_event "
                      "%u, attr_len(%u) mac_addr %2hx:%2hx:%2hx:%2hx:%2hx:%2hx",
                      attr_type, attr_event, attr_len, mac_addr[0],
                      mac_addr[1], mac_addr[2], mac_addr[3], mac_addr[4], 
                      mac_addr[5]);
        }

      /* Set the attribute value */
      /* leave all contains only attribute buff_len and attribute event */
      gmrp_set_octets (buff_ptr, buff_len, offset, mac_addr, 
                       GMRP_ATTRIBUTE_VALUE_GROUP_MEMBERSHIP_SIZE);
    } 
  else if (attr_type == GMRP_ATTRIBUTE_TYPE_SERVICE_REQUIREMENT)
    { /* attribute value = 1 byte Service Requirement */
      /* leave all contains only attribute buff_len and attribute event */
      u_char attr_val = GMRP_SERVICE_REQUIREMENT_ALL_GROUPS;

      gmrp_set_octets (buff_ptr, buff_len, offset, &attr_val, 
                       GMRP_ATTRIBUTE_VALUE_SERVICE_REQUIREMENT_SIZE);
    }
}

static void
gmrp_gid_delete_entries (struct gmrp_gmd *gmd, struct gid *gid)
{
  struct ptree_node *rn;
  struct gmrp_attr_entry_tbd *attr_entry_tbd;

  if (!gmd || !gmd->to_be_deleted_attr_table || !gid)
    return;

  for (rn = ptree_top (gmd->to_be_deleted_attr_table); rn;
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
gmrp_gmd_delete_entries (struct gmrp_gmd *gmd, struct gid *gid)
{
  struct ptree_node *rn;
  struct gmrp_attr_entry_tbd *attr_entry_tbd;

  if (!gmd || !gmd->to_be_deleted_attr_table || !gid)
    return;

  for (rn = ptree_top (gmd->to_be_deleted_attr_table); rn;
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

          gmd_delete_entry (gmd, attr_entry_tbd->attribute_index);

          /* Delete the entry in to_be_deleted_attr_table */
          PREFIX_ATTRIBUTE_INFO_UNSET (rn);
          XFREE (MTYPE_GMRP_GMD_ENTRY, attr_entry_tbd);
        }
    }
  return;
}

static inline bool_t
gmrp_build_message (struct gmrp *gmrp, struct gid *gid, 
                    u_char *buff_ptr, u_int32_t *buff_len, 
                    u_int32_t *offset)
{
  u_char attr_type;
  u_char temp_attr_type;
  u_char group_mem_attr_type = GMRP_ATTRIBUTE_TYPE_GROUP_MEMBERSHIP;
  u_char event;
  u_int32_t gid_index;
  u_int32_t attr_index = 0;
  u_int32_t start_index = 0;
  u_char leave_all_flag = 0;
  u_char leave_all_transmitted = 0;
  struct gid_port *gid_port = NULL;

  if ((!gid) || (!(gid_port = gid->gid_port)))
   return attr_index;

  if ((event = gid_tx_attribute (gid, start_index, &gid_index)) != 
      GID_EVENT_NULL)
    {
      if (event == GID_EVENT_TX_LEAVE_ALL)
        leave_all_flag = 1;

      /* Find the attribute type */ 
      attr_type = gmrp_get_attribute_type (gid_index);

      /* Set the attribute end mark */
      gmrp_set_octets (buff_ptr, buff_len, offset, &attr_type,
                       GARP_ATTRIBUTE_TYPE_SIZE);

      gmrp_build_attribute (gmrp, buff_ptr, buff_len, offset,
                            attr_type, gid_index, event);

      attr_index++;

      while ((*buff_len > GMRP_GROUP_MEMBERSHIP_NON_LEAVE_ALL_SIZE) &&
             (attr_index < GMRP_MAX_ATTR_PER_PDU) &&
             ((event = gid_tx_attribute (gid, start_index, &gid_index))
                                         != GID_EVENT_NULL))
        {
          if (leave_all_flag)
            {
              if ((!leave_all_transmitted) &&
                 (gid_index >= GMRP_NUMBER_OF_LEGACY_CONTROL))
                {
                   u_char end_mark = GARP_ATTRIBUTE_END_MARK;
                   gmrp_set_octets (buff_ptr, buff_len, offset, &end_mark,
                                    GARP_ATTRIBUTE_END_MARK_SIZE);

                  /* If the event is LeaveAll and it is not already transmitted
                   * then set the LeaveAll message for Group Membership 
                   */

                  /* Set the attribute end mark */
                  gmrp_set_octets (buff_ptr, buff_len, offset, 
                                   &group_mem_attr_type, 
                                   GARP_ATTRIBUTE_TYPE_SIZE);
                  gmrp_build_attribute (gmrp, buff_ptr, buff_len, offset,
                                        group_mem_attr_type, gid_index, 
                                        GID_EVENT_TX_LEAVE_ALL);

                  attr_index++;
                  leave_all_transmitted = 1;
                }

              if (event == GID_EVENT_TX_EMPTY)
                continue;
            }

          temp_attr_type = gmrp_get_attribute_type (gid_index);

          if ( (temp_attr_type != attr_type) && (!leave_all_flag) )
            { /* Set the End Mark Attribute */
              u_char end_mark = GARP_ATTRIBUTE_END_MARK;
              gmrp_set_octets (buff_ptr, buff_len, offset, &end_mark,
                               GARP_ATTRIBUTE_END_MARK_SIZE);

              attr_type = temp_attr_type;
              gmrp_set_octets (buff_ptr, buff_len, offset, &attr_type,
                               GARP_ATTRIBUTE_TYPE_SIZE);
            }
          else
            {
              /* Update the attribute type */
              attr_type = temp_attr_type;
            }

          gmrp_build_attribute (gmrp, buff_ptr, buff_len, offset,
                                attr_type, gid_index, event);
          attr_index++;
        }
    }

  /* Even when there are no gmrp group membership attributes registered 
     we should send a LeaveAll message */

  if (leave_all_flag && (!leave_all_transmitted))
    {
      /* Set the attribute end mark */
      u_char end_mark = GARP_ATTRIBUTE_END_MARK;
      gmrp_set_octets (buff_ptr, buff_len, offset, &end_mark,
                       GARP_ATTRIBUTE_END_MARK_SIZE);
      gmrp_set_octets (buff_ptr, buff_len, offset,
                       &group_mem_attr_type,
                       GARP_ATTRIBUTE_TYPE_SIZE);
      gmrp_build_attribute (gmrp, buff_ptr, buff_len, offset,
                            group_mem_attr_type,
                            gid_index, GID_EVENT_TX_LEAVE_ALL);
      attr_index++;
    }


  if (attr_index)
    {
      /* Set the End Mark Attribute */
      u_char end_mark = GARP_ATTRIBUTE_END_MARK;
      gmrp_set_octets (buff_ptr, buff_len, offset, &end_mark,
                       GARP_ATTRIBUTE_END_MARK_SIZE);

      if (GMRP_DEBUG(PACKET))
        {
          zlog_debug (gmrpm, "gmrp_build_message: [port %d] Number of "
                      "attributes in the message %u", gid_port->ifindex, attr_index);
        }
    }
  
  /* Check for any attributes to be deleted and delete those */
  gmrp_gid_delete_entries (gmrp->gmd, gid);
  gmrp_gmd_delete_entries (gmrp->gmd, gid);

  return attr_index;
}

#ifdef HAVE_MMRP

static inline bool_t
mmrp_build_message (struct gmrp *gmrp, struct gid *gid,
                    u_char *buff_ptr, u_int32_t *buff_len,
                    u_int32_t *offset)
{
  u_char event;
  u_char attr_event;
  u_int32_t gid_index;
  u_int32_t vector = 0;
  u_char encode_first_val;
  u_int8_t p2p_vector = 0;
  u_int32_t attr_index = 0;
  u_int32_t start_index = 0;
  u_int16_t no_of_values = 0;
  u_int32_t no_of_val_offset;
  u_char mac_addr[HAL_HW_LENGTH];
  struct gid_port *gid_port = NULL;
  u_char vector_octet [MMRP_VECTOR_SIZE];
  u_int8_t tx_event = MRP_TX_EVENT_NULL;
  u_char end_mark = GARP_ATTRIBUTE_END_MARK;
  u_char attr_type = GMRP_ATTRIBUTE_TYPE_NONE;
  u_char no_of_val_octet [MMRP_NUM_OF_VALUES_SIZE]; 
  u_char temp_attr_type = GMRP_ATTRIBUTE_TYPE_NONE;
  u_int8_t leave_all_event = MRP_LEAVE_ALL_EVENT_NULL;

  if ((!gid) || (!(gid_port = gid->gid_port)))
   return attr_index;
  pal_mem_set (gmrp->mac_addr, 0, HAL_HW_LENGTH);

  encode_first_val = PAL_FALSE;

  no_of_val_offset = *offset;

  mad_get_tx_leave_all_event (gid, &tx_event, &leave_all_event);

  while (((event = mad_tx_attribute (gid, start_index, &gid_index, 
                                     tx_event)) != GID_EVENT_NULL))
    {
       if (*offset >= (GMRP_PDU_SIZE - 
                       GMRP_GROUP_MEMBERSHIP_NON_LEAVE_ALL_SIZE))
           break;
       /* Find the attribute type */
       temp_attr_type = gmrp_get_attribute_type (gid_index);

       attr_index++;

       if (attr_type != temp_attr_type)
         {
           if (no_of_values > 0)
             {
                if (gid_port->p2p == PAL_TRUE)
                  {
                    if (no_of_values < MRP_NUM_PACKED_EVENT_P2P)
                      {
                        gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_NULL]
                              += (MRP_NUM_PACKED_EVENT_P2P - no_of_values);
                        gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_MAX]
                              += (MRP_NUM_PACKED_EVENT_P2P - no_of_values);

                        mrp_add_null_events (&vector, no_of_values,
                                             MRP_NUM_PACKED_EVENT_P2P,
                                             MRP_VEC_MUL_FAC_P2P);
                      }

                    p2p_vector = vector & 0xFF;

                    gmrp_set_octets (buff_ptr, buff_len, offset, &p2p_vector,
                                     MRP_COMPACT_VECTOR_SIZE);

                    no_of_values = MRP_NUM_PACKED_EVENT_P2P;
                  }
                else
                  {
                    if (no_of_values < MRP_NUM_PACKED_EVENT_NOT_P2P)
                      {
                        gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_NULL]
                              += (MRP_NUM_PACKED_EVENT_NOT_P2P - no_of_values);
                        gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_MAX]
                              += (MRP_NUM_PACKED_EVENT_NOT_P2P - no_of_values);

                        mrp_add_null_events (&vector, no_of_values,
                                             MRP_NUM_PACKED_EVENT_NOT_P2P,
                                             MRP_VEC_MUL_FAC_NOT_P2P);
                      }

                    mrp_vector_to_octets (&vector, vector_octet);
                    gmrp_set_octets (buff_ptr, buff_len, offset, vector_octet,
                                     MRP_VECTOR_SIZE);
                    no_of_values = MRP_NUM_PACKED_EVENT_NOT_P2P;
                  }

               no_of_values = no_of_values +
                              leave_all_event * (MRP_NUM_ATTR_EVENT + 1);

               mrp_no_of_val_to_octets (&no_of_values, no_of_val_octet);

               gmrp_set_octets (buff_ptr, buff_len, &no_of_val_offset,
                                no_of_val_octet, MMRP_NUM_OF_VALUES_SIZE);

               vector = 0;
             }

            end_mark = GARP_ATTRIBUTE_END_MARK;

            if (attr_type != GMRP_ATTRIBUTE_TYPE_NONE)
              gmrp_set_octets (buff_ptr, buff_len, offset, &end_mark,
                               GARP_ATTRIBUTE_END_MARK_SIZE);
 
            attr_type = temp_attr_type;

            /* Set the attribute type */
            gmrp_set_octets (buff_ptr, buff_len, offset, &attr_type,
                             GARP_ATTRIBUTE_TYPE_SIZE);

            encode_first_val = PAL_TRUE;

            no_of_val_offset = *offset;
            no_of_values = 0;

            /* Fill the no_of_values with zero for now */
            mrp_no_of_val_to_octets (&no_of_values, no_of_val_octet);
            gmrp_set_octets (buff_ptr, buff_len, offset, no_of_val_octet,
                             MMRP_NUM_OF_VALUES_SIZE);
         }

       if ((temp_attr_type = gmrp_get_attribute_type (gid_index))
                           == GMRP_ATTRIBUTE_TYPE_GROUP_MEMBERSHIP)
         {
           gmd_get_mac_addr (gmrp->gmd, gid_index, mac_addr);

           if (! gmrp_check_mac(gmrp, mac_addr)
               || encode_first_val == PAL_TRUE)
             {

               pal_mem_cpy (gmrp->mac_addr, mac_addr, HAL_HW_LENGTH);
               encode_first_val = PAL_FALSE;

               if (no_of_values > 0)
                 {
                    if (gid_port->p2p == PAL_TRUE)
                      {
                        if (no_of_values < MRP_NUM_PACKED_EVENT_P2P)
                          {
                            gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_NULL]
                              += (MRP_NUM_PACKED_EVENT_P2P - no_of_values);
                            gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_MAX]
                              += (MRP_NUM_PACKED_EVENT_P2P - no_of_values);

                            mrp_add_null_events (&vector, no_of_values,
                                                 MRP_NUM_PACKED_EVENT_P2P,
                                                 MRP_VEC_MUL_FAC_P2P);
                          }

                        p2p_vector = vector & 0xFF;

                        gmrp_set_octets (buff_ptr, buff_len, offset,
                                         &p2p_vector, MRP_COMPACT_VECTOR_SIZE);

                        no_of_values = MRP_NUM_PACKED_EVENT_P2P;
                      }
                    else if (gid_port->p2p != PAL_TRUE)
                      {
                        if (no_of_values < MRP_NUM_PACKED_EVENT_NOT_P2P)
                          {
                            gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_NULL]
                              += (MRP_NUM_PACKED_EVENT_NOT_P2P - no_of_values);
                            gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_MAX]
                              += (MRP_NUM_PACKED_EVENT_NOT_P2P - no_of_values);

                            mrp_add_null_events (&vector, no_of_values,
                                                 MRP_NUM_PACKED_EVENT_NOT_P2P,
                                                 MRP_VEC_MUL_FAC_NOT_P2P);
                          }

                         mrp_vector_to_octets (&vector, vector_octet);
                         gmrp_set_octets (buff_ptr, buff_len, offset,
                                          vector_octet, MRP_VECTOR_SIZE);
                         no_of_values = MRP_NUM_PACKED_EVENT_NOT_P2P;
                      }

                   no_of_values = no_of_values + 
                                  leave_all_event * (MRP_NUM_ATTR_EVENT + 1);

                   mrp_no_of_val_to_octets (&no_of_values, no_of_val_octet);
                   gmrp_set_octets (buff_ptr, buff_len, &no_of_val_offset,
                                    no_of_val_octet, MMRP_NUM_OF_VALUES_SIZE);

                   no_of_val_offset = *offset;
                   vector = 0;
                   no_of_values = 0;

                   /* Fill the no_of_values with zero for now */

                   mrp_no_of_val_to_octets (&no_of_values, no_of_val_octet);
                   gmrp_set_octets (buff_ptr, buff_len, offset, no_of_val_octet,
                                    MMRP_NUM_OF_VALUES_SIZE);
                 }

               gmrp_set_octets (buff_ptr, buff_len, offset, mac_addr,
                                GMRP_ATTRIBUTE_VALUE_GROUP_MEMBERSHIP_SIZE);

               no_of_values += 1;

               if (gid_port->p2p == PAL_TRUE)
                 attr_event = mrp_map_gid_event_to_p2p_attribute_event (event);
               else
                 attr_event = mrp_map_gid_event_to_attribute_event (event);

               gmrp->garp_instance.transmit_counters[attr_event]++;
               gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_MAX]++;

               if (gid_port->p2p == PAL_TRUE)
                 vector = (vector * MRP_VEC_MUL_FAC_P2P) + attr_event;
               else
                 vector = (vector * MRP_VEC_MUL_FAC_NOT_P2P) + attr_event;

               attr_index++;
             }
           else
             {
                if (gid_port->p2p == PAL_TRUE
                    && no_of_values == MRP_NUM_PACKED_EVENT_P2P)
                  {
                    p2p_vector = vector & 0xFF;

                    gmrp_set_octets (buff_ptr, buff_len, offset,
                                     &p2p_vector, MRP_COMPACT_VECTOR_SIZE);

                    vector = 0;
                  }
                else if (gid_port->p2p != PAL_TRUE
                         && no_of_values == MRP_NUM_PACKED_EVENT_NOT_P2P)
                  {
                    mrp_vector_to_octets (&vector, vector_octet);
                    gmrp_set_octets (buff_ptr, buff_len, offset,
                                     vector_octet, MRP_VECTOR_SIZE);
                    vector = 0;
                  }

               if (gid_port->p2p == PAL_TRUE)
                 attr_event = mrp_map_gid_event_to_p2p_attribute_event (event);
               else
                 attr_event = mrp_map_gid_event_to_attribute_event (event);

               gmrp->garp_instance.transmit_counters[attr_event]++;
               gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_MAX]++;

               if (gid_port->p2p == PAL_TRUE)
                 vector = (vector * MRP_VEC_MUL_FAC_P2P) + attr_event;
               else
                 vector = (vector * MRP_VEC_MUL_FAC_NOT_P2P) + attr_event;

                no_of_values += 1;
             }
         }
       else
         {
           u_char attr_val = GMRP_SERVICE_REQUIREMENT_ALL_GROUPS;

           if (encode_first_val == PAL_TRUE)
             {
               gmrp_set_octets (buff_ptr, buff_len, offset, &attr_val,
                                GMRP_ATTRIBUTE_VALUE_SERVICE_REQUIREMENT_SIZE);
               encode_first_val = PAL_FALSE;
             }

           if (gid_port->p2p == PAL_TRUE)
             attr_event = mrp_map_gid_event_to_p2p_attribute_event (event);
           else
             attr_event = mrp_map_gid_event_to_attribute_event (event);

           gmrp->garp_instance.transmit_counters[attr_event]++;
           gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_MAX]++;
 
           no_of_values += 1;

           if (gid_port->p2p == PAL_TRUE)
             vector = (vector * MRP_VEC_MUL_FAC_P2P) + attr_event;
           else
             vector = (vector * MRP_VEC_MUL_FAC_NOT_P2P) + attr_event;
         }
    }

  if (no_of_values > 0)
    {
      if (gid_port->p2p == PAL_TRUE)
        {
          if (no_of_values < MRP_NUM_PACKED_EVENT_P2P)
            {
              gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_NULL]
                    += (MRP_NUM_PACKED_EVENT_P2P - no_of_values);
              gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_MAX]
                    += (MRP_NUM_PACKED_EVENT_P2P - no_of_values);

              mrp_add_null_events (&vector, no_of_values,
                                   MRP_NUM_PACKED_EVENT_P2P,
                                   MRP_VEC_MUL_FAC_P2P);
            }

          p2p_vector = vector & 0xFF;

          gmrp_set_octets (buff_ptr, buff_len, offset,
                           &p2p_vector, MRP_COMPACT_VECTOR_SIZE);

          no_of_values = MRP_NUM_PACKED_EVENT_P2P;
        }
      else
        {
          if (no_of_values < MRP_NUM_PACKED_EVENT_NOT_P2P)
            {
              gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_NULL]
                    += (MRP_NUM_PACKED_EVENT_NOT_P2P - no_of_values);
              gmrp->garp_instance.transmit_counters[MRP_ATTR_EVENT_MAX]
                    += (MRP_NUM_PACKED_EVENT_NOT_P2P - no_of_values);

              mrp_add_null_events (&vector, no_of_values,
                                   MRP_NUM_PACKED_EVENT_NOT_P2P,
                                   MRP_VEC_MUL_FAC_NOT_P2P);
            }

          mrp_vector_to_octets (&vector, vector_octet);
          gmrp_set_octets (buff_ptr, buff_len, offset,
                           vector_octet, MRP_VECTOR_SIZE);
          no_of_values = MRP_NUM_PACKED_EVENT_NOT_P2P;
        }

      no_of_values = no_of_values +
                     leave_all_event * (MRP_NUM_ATTR_EVENT + 1);

      mrp_no_of_val_to_octets (&no_of_values, no_of_val_octet);
      gmrp_set_octets (buff_ptr, buff_len, &no_of_val_offset,
                       no_of_val_octet, MMRP_NUM_OF_VALUES_SIZE);

      vector = 0;
    }

  /* Set the attribute end mark */
  gmrp_set_octets (buff_ptr, buff_len, offset, &end_mark,
                   GARP_ATTRIBUTE_END_MARK_SIZE);


  /* Set the End Mark Attribute */
  gmrp_set_octets (buff_ptr, buff_len, offset, &end_mark,
                   GARP_ATTRIBUTE_END_MARK_SIZE);

  if (GMRP_DEBUG(PACKET))
    {
      zlog_debug (gmrpm, "gmrp_build_message: [port %d] Number of "
                  "attributes in the message %u", gid_port->ifindex,
                  attr_index);
    }
 
  /* Check for any attributes to be deleted and delete those */
  gmrp_gid_delete_entries (gmrp->gmd, gid);
  gmrp_gmd_delete_entries (gmrp->gmd, gid);

  return attr_index;
}

#endif /* HAVE_MMRP */

static inline bool_t
gmrp_build_pdu (struct gmrp *gmrp, struct gid *gid, 
                u_char *buff_ptr, u_int32_t *buff_len)
{
  u_int32_t offset = 0;
  u_char llc_format[3] = {0x42, 0x42, 0x03};
  u_char protocolID [GARP_PROTOCOL_ID_SIZE];
  u_char min_len;
  u_int32_t message_index = 0;
  struct gid_port *gid_port = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;

  if ((!gid) || (!(gid_port = gid->gid_port))
      || (!(gmrp_bridge = gmrp->gmrp_bridge)))
   return message_index;

#ifdef HAVE_MMRP
  if (gmrp_bridge->reg_type == REG_TYPE_MMRP)
    min_len = GARP_ATTRIBUTE_TYPE_SIZE + GARP_NO_OF_VALUES_SIZE + 
              GMRP_GROUP_MEMBERSHIP_NON_LEAVE_ALL_SIZE +
              GARP_VECTOR_SIZE + GARP_ATTRIBUTE_END_MARK_SIZE + 
              GARP_PDU_END_MARK;
  else
#endif /* HAVE_MMRP */
    min_len = GARP_ATTRIBUTE_TYPE_SIZE + GARP_ATTRIBUTE_LENGTH_SIZE +
              GMRP_GROUP_MEMBERSHIP_NON_LEAVE_ALL_SIZE + 
              GARP_ATTRIBUTE_END_MARK_SIZE + GARP_PDU_END_MARK;
#ifdef HAVE_MMRP

  if (gmrp_bridge->reg_type == REG_TYPE_MMRP)
    {
      /* Set the Protocol version */
      protocolID[0] = 0x00;

      gmrp_set_octets (buff_ptr, buff_len, &offset, protocolID, 
                       MRP_PROTOCOL_ID_SIZE); /* This is the protocol version in
                                                 in case of MMRP */
    }
  else
#endif /* HAVE_MMRP */
    {
      /* Set the LLC encapsulation */
      gmrp_set_octets (buff_ptr, buff_len, &offset, llc_format, 3);

      protocolID[0] = 0x00;
      protocolID[1] = 0x01;

      gmrp_set_octets (buff_ptr, buff_len, &offset, protocolID, 
                       GARP_PROTOCOL_ID_SIZE);
   }

  while (*buff_len > min_len)
    {
#ifdef HAVE_MMRP    
      if (gmrp_bridge->reg_type == REG_TYPE_MMRP)
        {
          if (!mmrp_build_message (gmrp, gid, buff_ptr, buff_len, &offset))
            {
              break;
            }
        }
      else
#endif /* HAVE_MMRP */
        {
          if (!gmrp_build_message (gmrp, gid, buff_ptr, buff_len, &offset))
            {
              break;
            }
        }
       message_index++;
    }

  /* Set the End Mark Pdu */
  if (message_index)
    {
      u_char end_mark = GARP_PDU_END_MARK;
      gmrp_set_octets (buff_ptr, buff_len, &offset, &end_mark, 
                       GARP_PDU_END_MARK_SIZE);

      if (GMRP_DEBUG(PACKET))
        {
          zlog_debug (gmrpm, "gmrp_build_pdu: [port %d] Number of messages "
                      "in the pdu %u", gid_port->ifindex, message_index);
        }
    }

  return message_index;
}

static inline bool_t 
gmrp_parse_attribute (struct gmrp *gmrp, struct gid *gid,
                      u_char *buff_ptr, u_int32_t *buff_len, 
                      u_int32_t *offset, u_char attr_type)
{ 
  enum gid_event event;
  u_int32_t attribute_index;
  u_int32_t starting_gid_index = 0;
  u_int32_t ending_gid_index = 0;
  u_char octet;
  u_char mac_addr[HAL_HW_LENGTH];
  struct gid_port *gid_port = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;

  if ((!gid) || (!(gid_port = gid->gid_port))||
      !(gmrp_bridge = gmrp->gmrp_bridge))
   return PAL_FALSE;

  /* Get the attribute buff_len */
  gmrp_get_octets (buff_ptr, buff_len, offset, &octet, 
                   GARP_ATTRIBUTE_LENGTH_SIZE);

  /* Check if the attribute buff_len is between min and max buff_len */
  /* Eventhough the max buff_len is not checked any value greater than max
   * will cause an overflow and thus will result lesser than the min */
  if (octet < GARP_ATTRIBUTE_LENGTH_MIN)
    {
      return PAL_FALSE; 
    }

  /* Get the attribute event */
  gmrp_get_octets (buff_ptr, buff_len, offset, &octet, 
                   GARP_ATTRIBUTE_EVENT_SIZE);

  /* Check if the event value is is lesser than GARP_ATTR_EVENT_MAX */
  if (octet > GARP_ATTR_EVENT_MAX)
    {
      return PAL_FALSE;
    }

  /* Convert the event received to actual gid events */
  event = garp_map_attribute_event_to_gid_event (octet);

  /* Incrememt the counter */
  gmrp->garp_instance.receive_counters[octet]++;
  gmrp->garp_instance.receive_counters[TOTAL_PACKET_COUNT]++;

  /* Get the attribute value  */
  if (event == GID_EVENT_RCV_LEAVE_ALL)
    { 
      if (attr_type == GMRP_ATTRIBUTE_TYPE_GROUP_MEMBERSHIP)
        {
          starting_gid_index = GMRP_NUMBER_OF_LEGACY_CONTROL;
          ending_gid_index = 0;
        }
      else if (attr_type == GMRP_ATTRIBUTE_TYPE_SERVICE_REQUIREMENT)
        {
          starting_gid_index = 0;
          ending_gid_index = GMRP_NUMBER_OF_LEGACY_CONTROL - 1;
        }

      gid_rcv_leave_all (gid, starting_gid_index, ending_gid_index);
      
      return PAL_TRUE;     
    }

  if (attr_type == GMRP_ATTRIBUTE_TYPE_GROUP_MEMBERSHIP)
    { /* attribute value = 6 bytes mac address for group attributes */
      gmrp_get_octets (buff_ptr, buff_len, offset, mac_addr, 
                       GMRP_ATTRIBUTE_VALUE_GROUP_MEMBERSHIP_SIZE);

      /* Get the gid_index associated with the mac address from the GMD */
      if (!gmd_find_entry(gmrp->gmd, mac_addr, &attribute_index))
        {
          if (!gmd_create_entry (gmrp->gmd, mac_addr, &attribute_index))
            {
              zlog_debug (gmrpm, "gmrp_parse_attribute: [port %d]"
                          "ERROR: DATABASE FULL", gid_port->ifindex);
            }
        }
    }
  else if (attr_type == GMRP_ATTRIBUTE_TYPE_SERVICE_REQUIREMENT)
    { /* attribute value = 1 byte for service requirement */
      gmrp_get_octets (buff_ptr, buff_len, offset, &octet, 
                       GMRP_ATTRIBUTE_VALUE_SERVICE_REQUIREMENT_SIZE);
      attribute_index = octet;
    }

  /* Call the GARP layer to handle the port, attribute, event */
  /* If a join-in is received for a mac-address that is already configured
   * on the port, then the GID must not be entered in the
   * Registrar State Machine.
   */
  if ( ! (nsm_is_mac_present (gmrp_bridge->bridge, mac_addr, gmrp->vlanid,
                              gmrp->svlanid, CLI_MAC, gid_port->ifindex)) )
    gid_rcv_attribute (gid, attribute_index, event);

  return PAL_TRUE;
}

#ifdef HAVE_MMRP

static inline bool_t
mmrp_parse_attribute (struct gmrp *gmrp, struct gid *gid,
                      u_char *buff_ptr, u_int32_t *buff_len,
                      u_int32_t *offset, u_char attr_type)
{
  enum gid_event event;
  u_int32_t attribute_index;
  u_int32_t starting_gid_index = 0;
  u_int32_t ending_gid_index = 0;
  u_char octet[4];
  s_int32_t increment = 0;
  u_int32_t vector;
  u_int32_t attr;
  u_int32_t index;
  u_int16_t no_of_values;
  u_int16_t no_of_vector;
  u_char mac_addr[HAL_HW_LENGTH];
  u_char event_mac_addr[HAL_HW_LENGTH];
  struct gid_port *gid_port = NULL;
  struct gmrp_bridge *gmrp_bridge = NULL;

  if ((!gid) || (!(gid_port = gid->gid_port))||
      !(gmrp_bridge = gmrp->gmrp_bridge))
   return PAL_FALSE;

  /* Get the attribute buff_len */
  gmrp_get_octets (buff_ptr, buff_len, offset, octet,
                   MMRP_NUM_OF_VALUES_SIZE);

  no_of_values = (octet[0] << 8);
  no_of_values |= octet[1];
  
  if ((attr = MRP_DECODE_LEAVE_ALL_VAL))
    {
      event = mrp_map_leave_all_event_to_gid_event (attr);

      if (attr_type == GMRP_ATTRIBUTE_TYPE_GROUP_MEMBERSHIP)
        {
          starting_gid_index = GMRP_NUMBER_OF_LEGACY_CONTROL;
          ending_gid_index = 0;
        }
      else if (attr_type == GMRP_ATTRIBUTE_TYPE_SERVICE_REQUIREMENT)
        {
          starting_gid_index = 0;
          ending_gid_index = GMRP_NUMBER_OF_LEGACY_CONTROL - 1;
        }

      gid_rcv_leave_all (gid, starting_gid_index, ending_gid_index);

    }

  no_of_values = no_of_values % (MRP_NUM_ATTR_EVENT + 1);

  if (gid_port->p2p)
    {
      increment = MRP_NUM_PACKED_EVENT_P2P - 1;
      no_of_vector = (no_of_values / MRP_NUM_PACKED_EVENT_P2P);
    }
  else 
    {
      increment = MRP_NUM_PACKED_EVENT_NOT_P2P - 1;
      no_of_vector = (no_of_values / MRP_NUM_PACKED_EVENT_NOT_P2P);
    }
 
  if (no_of_vector == 0)
    return PAL_FALSE;

  for (index = 0; index < no_of_vector; index++)   
     {
       if (attr_type == GMRP_ATTRIBUTE_TYPE_GROUP_MEMBERSHIP)
         {
           gmrp_get_octets (buff_ptr, buff_len, offset, mac_addr,
                            GMRP_ATTRIBUTE_VALUE_GROUP_MEMBERSHIP_SIZE);
           pal_mem_cpy(event_mac_addr, mac_addr, HAL_HW_LENGTH);
         }
       else
         {
           gmrp_get_octets (buff_ptr, buff_len, offset, octet,
                            GMRP_ATTRIBUTE_VALUE_SERVICE_REQUIREMENT_SIZE);
           attribute_index = octet[0];
         }

       /* Get the attribute event */
       if (gid_port->p2p == PAL_TRUE)
         {
           gmrp_get_octets (buff_ptr, buff_len, offset, octet,
                            MRP_COMPACT_VECTOR_SIZE);
           MMRP_DECODE_P2P_VECTOR;
         }
       else
         {
           gmrp_get_octets (buff_ptr, buff_len, offset, octet,
                            MRP_VECTOR_SIZE);
           MMRP_DECODE_NON_P2P_VECTOR;
         }
          
       while (increment >= 0)
         {
           if (gid_port->p2p)
             attr = vector % MRP_VEC_MUL_FAC_P2P;
           else
             attr = vector % MRP_VEC_MUL_FAC_NOT_P2P;
      
           /* Incrememt the counter */
           gmrp->garp_instance.receive_counters[attr]++;
           gmrp->garp_instance.receive_counters[MRP_ATTR_EVENT_MAX]++;

           /* Convert the event received to actual gid events */
           event = mrp_map_attribute_event_to_gid_event (attr);
         
           if (attr_type == GMRP_ATTRIBUTE_TYPE_GROUP_MEMBERSHIP)
             { 
               /* Get the gid_index associated with the mac address 
                  from the GMD */
               pal_mem_cpy (mac_addr, event_mac_addr, HAL_HW_LENGTH);

               mmrp_increment_mac (mac_addr, increment);

               if (!gmd_find_entry(gmrp->gmd, mac_addr, &attribute_index))
                 { 
                   if (event != GID_EVENT_NULL)
                     if (!gmd_create_entry (gmrp->gmd, mac_addr, 
                                            &attribute_index))
                       {
                         zlog_debug (gmrpm, "gmrp_parse_attribute: [port %d]"
                                     "ERROR: DATABASE FULL", gid_port->ifindex);
                       } 
                 }
             }
           else
             attribute_index += increment;

           /* Call the GARP layer to handle the port, attribute, event */

           /* If a join-in is received for a mac-address that is 
              already configured on the port, then the GID must 
              not be entered in the Registrar State Machine */
           if ( attr_type == GMRP_ATTRIBUTE_TYPE_GROUP_MEMBERSHIP
                && event != GID_EVENT_NULL
                && ! (nsm_is_mac_present (gmrp_bridge->bridge, 
                                          mac_addr, gmrp->vlanid,
                                          gmrp->svlanid,
                                          CLI_MAC, gid_port->ifindex)) )
             gid_rcv_attribute (gid, attribute_index, event);
           else if ( attr_type == GMRP_ATTRIBUTE_TYPE_SERVICE_REQUIREMENT
                     && attribute_index <= 
                        GMRP_SERVICE_REQUIREMENT_ALL_UNREGISTERED_GROUPS)
             gid_rcv_attribute (gid, attribute_index, event);

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

#endif /* HAVE_MMRP */

static void
gmrp_skip_attribute ( u_char *buff_ptr, u_int32_t *buff_len,
                      u_int32_t *offset)
{
  u_char octet;
  u_char mac_addr [GMRP_ATTRIBUTE_VALUE_GROUP_MEMBERSHIP_SIZE];
  enum gid_event event = GID_EVENT_MAX;
                                                                                                                             
  /* In this procedure it is made sure that the buff_len is reset to
   * 0 before returning if the message is not according to
   * GMRP packet format. This is to ensure that gmrp_parse_message
   * loop terminates in case of corrupted message.
   */
                                                
  if (*buff_len < GARP_ATTRIBUTE_LENGTH_SIZE)
    {
      *buff_len = 0;
      return;
    }
                                              
  gmrp_get_octets (buff_ptr, buff_len, offset, &octet,
                   GARP_ATTRIBUTE_LENGTH_SIZE);
                                             
  if (*buff_len < GARP_ATTRIBUTE_EVENT_SIZE)
    {
      *buff_len = 0;
      return;
    }

  gmrp_get_octets (buff_ptr, buff_len, offset, &octet,
                   GARP_ATTRIBUTE_EVENT_SIZE);

  /* Check if the event value is is lesser than GARP_ATTR_EVENT_MAX */
  if (octet > GARP_ATTR_EVENT_MAX)
    {
      *buff_len = 0;
      return;
    }
                                            
  /* Convert the event received to actual gid events */
  event = garp_map_attribute_event_to_gid_event (octet);
                                           
  if (event == GID_EVENT_RCV_LEAVE_ALL)
    {
      return;
    }

  if (*buff_len < GMRP_ATTRIBUTE_VALUE_GROUP_MEMBERSHIP_SIZE)
    {
      *buff_len = 0;
      return;
    }
                                          
  gmrp_get_octets (buff_ptr, buff_len, offset, mac_addr,
                   GMRP_ATTRIBUTE_VALUE_GROUP_MEMBERSHIP_SIZE);
  return;
}

static void
gmrp_skip_message ( u_char *buff_ptr, u_int32_t *buff_len,
                    u_int32_t *offset)
{
  u_char octet;
  
  while (buff_len)
    {
      gmrp_skip_attribute (buff_ptr, buff_len, offset);
 
      if (*buff_len < GARP_ATTRIBUTE_LENGTH_SIZE)
        {
          *buff_len = 0;
           return;
        }
      /* Get a single octet and check if we reached the end mark */
      gmrp_get_octets (buff_ptr, buff_len, offset, &octet,
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

  return;
}

static inline bool_t 
gmrp_parse_message (struct gmrp *gmrp, struct gid *gid, 
                    u_char *buff_ptr, u_int32_t *buff_len, 
                    u_int32_t *offset)
{
  u_char octet;
  u_char attr_type; 
  u_int32_t attr_index = 0;
  struct gmrp_bridge *gmrp_bridge;
  struct gid_port *gid_port = NULL;

  if ((!gid) || (!(gid_port = gid->gid_port))
      || !(gmrp_bridge = gmrp->gmrp_bridge))
   return PAL_FALSE;
  
  /* Get the attribute type */
  gmrp_get_octets (buff_ptr, buff_len, offset, &octet, 
                   GARP_ATTRIBUTE_TYPE_SIZE);
  attr_type = octet; 

  /* Check if the attribute type is Group membership or
   * service requirement.
   */

  if ((attr_type != GMRP_ATTRIBUTE_TYPE_GROUP_MEMBERSHIP) &&
      (attr_type != GMRP_ATTRIBUTE_TYPE_SERVICE_REQUIREMENT))
    {
      gmrp_skip_message (buff_ptr, buff_len, offset);
      zlog_debug (gmrpm, "gmrp_parse_message: [port %d] Invalid attribute type",
                  gid_port->ifindex);
      return PAL_TRUE;
    }

  while (*buff_len)
   {

#ifdef HAVE_MMRP
     if (gmrp_bridge->reg_type == REG_TYPE_MMRP)
       {
         if (!mmrp_parse_attribute (gmrp, gid, buff_ptr, buff_len, offset,
                                    attr_type))
           { /* Improper/invalid formatted attribute */
             zlog_debug (gmrpm, "mmrp_parse_message: [port %d] Error "
                         "while parsing attribute type %u num attributes parsed %u",
                         gid_port->ifindex, attr_type, attr_index);

             return PAL_FALSE;
           }
       }
     else
#endif /* HAVE_MMRP */
       {
         if (!gmrp_parse_attribute (gmrp, gid, buff_ptr, buff_len, offset,
                                    attr_type))
           { /* Improper/invalid formatted attribute */
             zlog_debug (gmrpm, "gmrp_parse_message: [port %d] Error "
                         "while parsing attribute type %u num attributes parsed %u",
                         gid_port->ifindex, attr_type, attr_index);

             return PAL_FALSE;
           }
       }

     attr_index++;

     /* Get a single octet and check if we reached the end mark */
     gmrp_get_octets (buff_ptr, buff_len, offset, &octet, 
                      GARP_ATTRIBUTE_LENGTH_SIZE);

     if (octet == GARP_ATTRIBUTE_END_MARK)
       {
         if (attr_index)
           {  /* Atleast one attribute in the message */
             if (GMRP_DEBUG(PACKET))
               {
                 zlog_debug (gmrpm, "gmrp_parse_message: [port %d] End of pdu "
                             "attribute type %u num attributes %u",
                              gid_port->ifindex, attr_type, attr_index);
               }
             break;
           }
         else
           {
             /* No attribute in the message */
             zlog_debug (gmrpm, "gmrp_parse_message: [port %d] Error "
                         "No attribute found, end of pdu", gid_port->ifindex);
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
  
  return PAL_TRUE;
}

void
gmrp_parse_pdu (struct gmrp *gmrp, struct gid *gid, u_char *buff_ptr, 
                u_int32_t buff_len)
{
  u_char octet;
  u_char protocolID[2];
  u_int32_t msg_index = 0;
  u_int32_t offset = 0;
  struct gmrp_bridge *gmrp_bridge;
  struct gid_port *gid_port = NULL;

  if ((!gid) || (!(gid_port = gid->gid_port))
      || !(gmrp_bridge = gmrp->gmrp_bridge))
   return;

  /* Get the protocolID */
#ifdef HAVE_MMRP
     if (gmrp_bridge->reg_type == REG_TYPE_MMRP)
       gmrp_get_octets (buff_ptr, &buff_len, &offset, protocolID,
                        MRP_PROTOCOL_ID_SIZE);
     else
#endif /* HAVE_MMRP */
       gmrp_get_octets (buff_ptr, &buff_len, &offset, protocolID,
                        GARP_PROTOCOL_ID_SIZE);

  while (buff_len)
    {
      if (!gmrp_parse_message (gmrp, gid, buff_ptr, &buff_len, &offset))
        { /* Improper/invalid formatted message */
          zlog_debug (gmrpm, "gmrp_parse_pdu: [port %d] Error while parsing "
                      "message", gid_port->ifindex);
          break;
        }

      msg_index++;

      if (buff_len == 0)
        break; 

      /* Get a single octet and check if we reached the end mark */
      gmrp_get_octets (buff_ptr, &buff_len, &offset, &octet, 
                       GARP_ATTRIBUTE_LENGTH_SIZE);

      if (octet == GARP_PDU_END_MARK)
        {
          if (msg_index)
            { /* Atleast one message in the pdu */
             if (GMRP_DEBUG(PACKET))
               {
                 zlog_debug (gmrpm, "gmrp_parse_pdu: [port %d] End of pdu "
                             "num messages %u", gid_port->ifindex, msg_index);
               }
              break; 
            }
          else
            { /* No message in the pdu */
              zlog_debug (gmrpm, "gmrp_parse_message: [port %d] Error no "
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
    } /* end of while */

  /* Inform gmrp to propagate the information received to all the other ports
     that are part of the bridge and the port state is forwarding */
  gmrp_do_actions (gid);
  
  if (GMRP_DEBUG(PACKET))
    {
      zlog_debug (gmrpm, "gmrp_parse_pdu: [port %d] Num messages in the pdu is "
                  "%u", gid_port->ifindex, msg_index);
    }

  return;
}


bool_t
gmrp_transmit_pdu (void *application, struct gid *gid)
{
  /* Build the pdu */
  u_char buff[GMRP_PDU_SIZE];
  u_char *bufptr = buff;
  u_int32_t buff_len, total_len;
  struct gmrp *gmrp;
  struct gid_port *gid_port = NULL;
  struct interface *ifp;
  struct apn_vr *vr;
  struct gmrp_bridge *gmrp_bridge = NULL;

  if ((!gid) || (!(gid_port = gid->gid_port)))
   return PAL_FALSE;

  gmrp = (struct gmrp*)application;

  if ((!gmrp) || !(gmrp_bridge = gmrp->gmrp_bridge))
    return PAL_FALSE;

  gmrp_bridge->gmrp_last_pdu_origin = gid_port->ifindex;

  if ((!gmrp) || (!(gmrp_bridge = gmrp->gmrp_bridge)))
    return PAL_FALSE;

  while (gid->tx_pending)
    {
      buff_len = GMRP_PDU_SIZE;
      pal_mem_set (buff, 0, GMRP_PDU_SIZE);
      total_len = 0;
      
      if (!gmrp_build_pdu (gmrp, gid, bufptr, &buff_len))
        {
          zlog_err (gmrpm, "GMRP build PDU failed \n");
          return PAL_FALSE;    
        }

      total_len += (GMRP_PDU_SIZE - buff_len);
  
      vr = apn_vr_get_privileged (gmrpm);
      if (vr == NULL)
        return PAL_FALSE;
                                                                          
      ifp = if_lookup_by_index (&vr->ifm, gid_port->ifindex);
      if (ifp == NULL)
        return PAL_FALSE;

      if (GMRP_DEBUG(PACKET))
        {  
          zlog_debug (gmrpm, "gmrp_transmit_pdu: [port %d] SENDING GMRP PDU "
                      "TO OTHER PARTICIPANTS ", gid_port->ifindex);
        }

      garp_send (gmrp_bridge->bridge->name, ifp->hw_addr, gmrp_dest_addr,
                 gid_port->ifindex, buff, total_len, gmrp->vlanid);
    }

  return PAL_TRUE;
}


void
gmrp_receive_pdu (unsigned int port_index, u_int8_t *src_mac,
                  u_char *buff, u_int32_t buff_len,
                  u_int16_t vid)
{
  struct gid *gid;
  struct gmrp *gmrp;
  struct nsm_if *zif;
  struct nsm_master *nm;
  struct interface *ifp;
  struct LLC_Frame_s llc;
  u_char *buff_ptr = buff;
  struct nsm_bridge *bridge;
  struct avl_node *node = NULL;
  struct avl_tree *gmrp_list = NULL;
  struct gmrp_bridge *gmrp_bridge;
  struct nsm_bridge_master *master;
  struct gmrp_port *gmrp_port = NULL;
  struct avl_tree *gmrp_port_list = NULL;
  struct nsm_bridge_port *br_port = NULL;
  struct apn_vr *vr = apn_vr_get_privileged (gmrpm);
  struct gmrp_port_instance *gmrp_port_instance = NULL;
  
  ifp = if_lookup_by_index(&vr->ifm, port_index);
  nm = vr->proto;
  master = nsm_bridge_get_master(nm);

  if ( (!ifp) || (!master) )
    return;
  
  zif = (struct nsm_if *)ifp->info;

  if ( ! zif)
    return;

  br_port = zif->switchport;

  if (! br_port)
    return;

  bridge = zif->bridge;

  if ( ! bridge )
    return;
  
  gmrp_list = bridge->gmrp_list;
  gmrp_port_list = br_port->gmrp_port_list;
  gmrp_port = br_port->gmrp_port;

  if ( (!gmrp_list) || (!gmrp_port_list) || (!gmrp_port) )
    {
      if (GMRP_DEBUG(PACKET))
        {
          zlog_debug (gmrpm, "gmrp_receive_pdu: [port %d] GMRP is not "
              "configured on bridge %s", ifp->ifindex, bridge->name);
        }
      return;
    }

  pal_mem_cpy (gmrp_port->gmrp_last_pdu_origin, src_mac, ETHER_ADDR_LEN);

  gmrp = gmrp_get_by_vid (gmrp_list, vid, vid, &node);

  gmrp_port_instance = gmrp_port_instance_get_by_vid (gmrp_port_list, 
                                                      vid, vid, &node);

  if ( (gmrp == NULL) || (gmrp_port_instance == NULL) )
    {
      if (GMRP_DEBUG(PACKET))
        {
          zlog_debug (gmrpm, "gmrp_receive_pdu: [port %d] GMRP is not "
                      "configured on this bridge", port_index);
        }
      return;
    }

  gmrp_bridge = gmrp->gmrp_bridge;

  if (gmrp_bridge == NULL)
    {
      if (GMRP_DEBUG(PACKET))
        {
          zlog_debug (gmrpm, "gmrp_receive_pdu: [port %d] GMRP is not "
                      "configured on this bridge", port_index);
        }

      return;
    }

  gid = gmrp_port_instance->gid;

  if (!gid)
    {
      if (GMRP_DEBUG(PACKET))
        {
          zlog_debug (gmrpm, "gmrp_receive_pdu: [port %d] GMRP is not "
                      "configured on this port", ifp->ifindex);
        }
      return;
    }

  /* Sec 12.9.1 Registrar administrative control values */
  /* If in forbidden mode ignore all garp messages */
  if (gmrp_port->registration_cfg == GID_REG_MGMT_FORBIDDEN )
    return;

  if (gmrp_bridge->reg_type == REG_TYPE_GMRP)
    {
      if (l2_llcParse (buff_ptr, &llc) == PAL_FALSE)
        return;

      buff_ptr += 3;
      buff_len -= 3;
    }

  gmrp_parse_pdu (gmrp, gid, buff_ptr, buff_len);

}

bool_t
gmrp_check_mac(struct gmrp *gmrp, u_char *mac_addr)
{

  if (gmrp->mac_addr[HAL_HW_LENGTH - 1] == 0xff)
    return PAL_FALSE;

  if (mac_addr [HAL_HW_LENGTH - 1] != gmrp->mac_addr[HAL_HW_LENGTH - 1] + 1)
    return PAL_FALSE;

  if (pal_mem_cmp (gmrp->mac_addr, mac_addr, HAL_HW_LENGTH - 1) == 0)
    return PAL_TRUE;
  else
    return PAL_FALSE;

  return PAL_FALSE;
}

#ifdef HAVE_MMRP

void
mmrp_increment_mac(u_char *mac, u_char increment)
{
 u_char inc;
 u_char index;

 for (inc = 0; inc < increment; inc++)
    {
      if (inc != 0)
        mac[5] += 1;

      index = 5;
  
      while (! mac[index])
        {
         mac[index] -= 1;
         mac[index - 1] += 1;
         index -= 1;
        }
     } 
 return;
}

void
mmrp_vector_to_octets (int *vector, u_char *octet)
{

  int shifted = *vector;
  int index,shift;

  shift = 1;

  for (index = 3; index >= 0; index--)
    {
      octet[index] = (shifted & 0xFF);
      shifted = (*vector >> (shift*8));
      shift++;
    }
}

#endif /* HAVE_MMRP */
