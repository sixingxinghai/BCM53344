/* Copyright (C) 2003  All Rights Reserved. */
#include "garp_gid.h"
#include "garp_pdu.h"
  
u_int32_t
garp_map_attribute_event_to_gid_event (enum garp_attribute_event event)
{
  enum gid_event new_event = 0;
  switch (event)
    {
      case GARP_ATTR_EVENT_LEAVE_ALL :
        new_event = GID_EVENT_RCV_LEAVE_ALL; 
        break;
      case GARP_ATTR_EVENT_JOIN_EMPTY :
        new_event = GID_EVENT_RCV_JOIN_EMPTY; 
        break;
      case GARP_ATTR_EVENT_JOIN_IN :
        new_event = GID_EVENT_RCV_JOIN_IN; 
        break;
      case GARP_ATTR_EVENT_LEAVE_EMPTY :
        new_event = GID_EVENT_RCV_LEAVE_EMPTY; 
        break;
      case GARP_ATTR_EVENT_LEAVE_IN :
        new_event = GID_EVENT_RCV_LEAVE_IN; 
        break;
      case GARP_ATTR_EVENT_EMPTY:
        new_event = GID_EVENT_RCV_EMPTY; 
        break;
      default:
        pal_assert (0);
        break;
    }

  return new_event;
}

#if defined HAVE_MVRP || defined HAVE_MMRP

u_int32_t
mrp_map_attribute_event_to_gid_event (enum mrp_attribute_event event)
{
   enum gid_event new_event = GID_EVENT_NULL;
   switch (event)
    {
      case MRP_ATTR_EVENT_NEW_IN:
        new_event = GID_EVENT_RCV_NEW_IN;
        break;
      case MRP_ATTR_EVENT_JOIN_IN:
        new_event = GID_EVENT_RCV_JOIN_IN;
        break;
      case MRP_ATTR_EVENT_LEAVE:
        new_event = GID_EVENT_RCV_LEAVE;
        break;
      case MRP_ATTR_EVENT_NULL:
        new_event = GID_EVENT_NULL;
        break;
      case MRP_ATTR_EVENT_NEW_EMPTY:
        new_event = GID_EVENT_RCV_NEW_EMPTY;
        break;
      case MRP_ATTR_EVENT_JOIN_EMPTY:
        new_event = GID_EVENT_RCV_JOIN_EMPTY;
        break;
      case MRP_ATTR_EVENT_EMPTY:
        new_event = GID_EVENT_RCV_EMPTY;
        break;
      default:
        new_event = GID_EVENT_NULL;
        break;
    }

  return new_event;
}

u_int32_t
mrp_map_leave_all_event_to_gid_event (enum mrp_leave_all_event event)
{
   enum gid_event new_event = 0;
   switch (event)
    {
      case MRP_LEAVE_ALL_EVENT_NULL:
        new_event = GID_EVENT_NULL;
        break;
      case MRP_LEAVE_ALL_EVENT_LEAVE_ALL:
        new_event = GID_EVENT_RCV_LEAVE_ALL;
        break;
      case MRP_LEAVE_ALL_EVENT_LEAVE_MINE:
        new_event = GID_EVENT_RCV_LEAVE_MINE;
        break;
      case MRP_LEAVE_ALL_EVENT_FULL:
        new_event = GID_EVENT_NULL;
        break;
      case MRP_LEAVE_ALL_EVENT_LEAVE_ALL_FULL:
        new_event = GID_EVENT_RCV_LEAVE_ALL;
      case MRP_LEAVE_ALL_EVENT_LEAVE_MINE_FULL:
        new_event = GID_EVENT_RCV_LEAVE_MINE;
        break;
      default:
        new_event = GID_EVENT_NULL;
        break;
    }

  return new_event;

}

#endif  /* HAVE_MMRP || HAVE_MVRP */ 

enum garp_attribute_event 
garp_map_gid_event_to_attribute_event (enum gid_event event)
{
  enum garp_attribute_event new_event;
  switch (event)
    {
      case GID_EVENT_TX_LEAVE_ALL:
        new_event = GARP_ATTR_EVENT_LEAVE_ALL;
        break;
      case GID_EVENT_TX_JOIN_EMPTY:
        new_event = GARP_ATTR_EVENT_JOIN_EMPTY;
        break;
      case GID_EVENT_TX_JOIN_IN:
        new_event = GARP_ATTR_EVENT_JOIN_IN; 
        break;
      case GID_EVENT_TX_LEAVE_EMPTY:
        new_event = GARP_ATTR_EVENT_LEAVE_EMPTY;
        break;
      case GID_EVENT_TX_LEAVE_IN:
        new_event = GARP_ATTR_EVENT_LEAVE_IN;
        break;
      case GID_EVENT_TX_EMPTY:
        new_event = GARP_ATTR_EVENT_EMPTY;
        break;
      default :
        pal_assert (0);
        break;
    }

  return new_event;
}

#if defined HAVE_MVRP || defined HAVE_MMRP

enum mrp_attribute_event
mrp_map_gid_event_to_p2p_attribute_event (enum gid_event event)
{
  enum mrp_attribute_event new_event;

  switch (event)
    {
      case GID_EVENT_TX_JOIN_IN:
      case GID_EVENT_TX_JOIN_IN_OPT:
      case GID_EVENT_TX_JOIN_EMPTY:
      case GID_EVENT_TX_JOIN_EMPTY_OPT:
        new_event = MRP_ATTR_EVENT_JOIN_IN;   /* JoinIn and Join have the
                                                 same attribute event value */
        break;
      case GID_EVENT_TX_NEW_IN:
      case GID_EVENT_TX_NEW_EMPTY:
        new_event = MRP_ATTR_EVENT_NEW_IN;   /* NewIn and New have the
                                                same attribute event value */
        break;
      case GID_EVENT_TX_LEAVE:
      case GID_EVENT_TX_LEAVE_OPT:
        new_event = MRP_ATTR_EVENT_LEAVE;
        break;
      case GID_EVENT_TX_NULL_OPT:
      case GID_EVENT_TX_EMPTY:
        new_event = MRP_ATTR_EVENT_NULL;
        break;
      default:
        new_event = MRP_ATTR_EVENT_NULL;
        break;
    }

  return new_event;
}

enum mrp_attribute_event
mrp_map_gid_event_to_attribute_event (enum gid_event event)
{
  enum mrp_attribute_event new_event;

  switch (event)
    {
      case GID_EVENT_TX_JOIN_IN:
      case GID_EVENT_TX_JOIN_IN_OPT:
        new_event = MRP_ATTR_EVENT_JOIN_IN;   /* JoinIn and Join have the
                                                 same attribute event value */
        break;
      case GID_EVENT_TX_NEW_IN:
        new_event = MRP_ATTR_EVENT_NEW_IN;   /* NewIn and New have the
                                                same attribute event value */
        break;
      case GID_EVENT_TX_LEAVE:
      case GID_EVENT_TX_LEAVE_OPT:
        new_event = MRP_ATTR_EVENT_LEAVE;
        break;
      case GID_EVENT_TX_NULL_OPT:
        new_event = MRP_ATTR_EVENT_NULL;
        break;
      case GID_EVENT_TX_EMPTY:
        new_event = MRP_ATTR_EVENT_EMPTY;
        break;
      case GID_EVENT_TX_NEW_EMPTY:
        new_event = MRP_ATTR_EVENT_NEW_EMPTY;
        break;
      case GID_EVENT_TX_JOIN_EMPTY:
      case GID_EVENT_TX_JOIN_EMPTY_OPT:
        new_event = MRP_ATTR_EVENT_JOIN_EMPTY;
        break;
      default:
        pal_assert (0);
        break;

    }

  return new_event;
}

enum mrp_attribute_event
mrp_map_leave_all_event_to_attribute_event (enum gid_event event)
{
  enum mrp_leave_all_event new_event;

  switch (event)
    {
      case GID_EVENT_TX_LEAVE_ALL:
        new_event = MRP_LEAVE_ALL_EVENT_LEAVE_ALL;
        break;
      case GID_EVENT_TX_LEAVE_ALL_FULL:
        new_event = MRP_LEAVE_ALL_EVENT_LEAVE_ALL_FULL;
        break;
      case GID_EVENT_TX_LEAVE_MINE:
        new_event = MRP_LEAVE_ALL_EVENT_LEAVE_MINE;
        break;
      case GID_EVENT_TX_LEAVE_MINE_FULL:
        new_event = MRP_LEAVE_ALL_EVENT_LEAVE_MINE_FULL;
        break;
      case GID_EVENT_TX_FULL:
        new_event = MRP_LEAVE_ALL_EVENT_FULL;
        break;
      default:
        new_event = MRP_LEAVE_ALL_EVENT_NULL;
        break;
    }

  return new_event; 
}

void
mrp_vector_to_octets (int *vector, u_char *octet)
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

void
mrp_add_null_events (u_int32_t *vector, u_int16_t start_index,
                     u_int16_t end_index, u_int16_t mul_fac)
{
  u_int32_t i;

  for (i = start_index + 1; i <= end_index; i++)
     *vector = (*vector * mul_fac) + MRP_ATTR_EVENT_NULL;
}

void
mrp_no_of_val_to_octets (short *no_of_val, u_char *octet)
{

  int shifted = *no_of_val;
  int index,shift;

  shift = 1;

  for (index = 1; index >= 0; index--)
    {
      octet[index] = (shifted & 0xFF);
      shifted = (*no_of_val >> (shift*8));
      shift++;
    }
}


#endif /* HAVE_MVRP || defined HAVE_MMRP */
