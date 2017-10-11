/*
  Forwarding database
  Linux ethernet bridge
  
  Authors:
  Lennert Buytenhek   <buytenh@gnu.org>
  
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the License, or (at your option) any later version.
*/
#include <linux/autoconf.h>
#include <linux/kernel.h>
#include <linux/spinlock.h>
#include <asm/atomic.h>
#include <asm/uaccess.h>
#include "if_ipifwd.h"
#include "br_types.h"
#include "br_fdb.h"
#include "bdebug.h"
#ifdef HAVE_MAC_AUTH
#include "auth.h"
#endif /* HAVE_MAC_AUTH */

extern unsigned char igmp_group_addr[];

#define WRAPAROUND_VALUE (0xffffffffUL / HZ + 1) /* HZ = frequency of ticks
                                                    per second. */

/*
  This function calculates the expiry time (in jiffies)
  of a dynamic fdb entry for the specified bridge. Note 
  that when a topology change is occurring, forward_delay 
  is used to calculate the expiry time.
  
  jiffies is declared extern in <sched.h>
*/

static __inline__ unsigned long
expiry_time ( const struct apn_bridge *const br )
{
  static unsigned long prev = 0;
  static unsigned long wraparound_count = 0;

  /* Check for wraparound. */
  if (prev > jiffies)
    wraparound_count++;

  prev = jiffies;

  return ((wraparound_count * WRAPAROUND_VALUE) + jiffies) - br->ageing_time;
}

static __inline__ int
has_expired ( struct apn_bridge *br, struct apn_bridge_fdb_entry *fdb )
{
  return ( !fdb->is_local
           && time_before_eq ( fdb->ageing_timestamp, expiry_time ( br ) ) );
}

/*
  This function is used to copy an entry for use in user space.
  It converts the internal timer from jiffies into seconds.
*/

static __inline__ void
copy_fdb ( struct fdb_entry *ent, struct apn_bridge_fdb_entry *f )
{
  int timer_expiry;
  static unsigned long prev = 0;
  static unsigned long wraparound_count = 0;
                                                                                
  /* Check for wraparound. */
  if (prev > jiffies)
    wraparound_count++;
                                                                                
  prev = jiffies;

  if (f->port && f->port->br)
    timer_expiry = f->port->br->ageing_time  - (((wraparound_count * WRAPAROUND_VALUE) + jiffies) - f->ageing_timestamp);
  memcpy ( ent->mac_addr, f->addr.addr, ETH_ALEN );
  ent->port_no = f->port ? f->port->port_no : 0;
  ent->is_local = f->is_local;
  ent->is_fwd = f->is_fwd;
  ent->vid = f->vid;
  ent->svid = f->svid;
  ent->ageing_timer_value = ( f->is_local
                              || f->is_static ) ? 0 : ( timer_expiry >
                                                        0 ) ? timer_expiry /
    HZ : 0;
}

/*
  This function is used to copy an entry for use in user space.
  It converts the internal timer from jiffies into seconds.
*/

static __inline__ void
copy_hal_fdb ( struct hal_fdb_entry *ent, struct apn_bridge_fdb_entry *f )
{
  int timer_expiry;
  static unsigned long prev = 0;
  static unsigned long wraparound_count = 0;

  /* Check for wraparound. */
  if (prev > jiffies)
    wraparound_count++;

  prev = jiffies;

  if (f->port && f->port->br)
    timer_expiry = f->port->br->ageing_time  - (((wraparound_count * WRAPAROUND_VALUE) + jiffies) - f->ageing_timestamp);
  memcpy ( ent->mac_addr, f->addr.addr, ETH_ALEN );
  ent->port         = f->port ? f->port->port_no : 0;
  ent->is_static    = f->is_static;
  ent->is_local     = f->is_local;
  ent->is_forward   = f->is_fwd;
  ent->vid          = f->vid;
  ent->svid         = f->svid;
  ent->ageing_timer_value = ( f->is_local
                              || f->is_static ) ? 0 : ( timer_expiry >
                                                        0 ) ? timer_expiry /
    HZ : 0;
}


/* The hashXXX routines needs to be placed in a 
   separate file where it is distinguished as customer 
   modifiable code.
*/

static __inline__ int
br_mac_hash ( unsigned char *mac )
{
  unsigned long x;
  int ret;

  x = mac[0];
  x = ( x << 2 ) ^ mac[1];
  x = ( x << 2 ) ^ mac[2];
  x = ( x << 2 ) ^ mac[3];
  x = ( x << 2 ) ^ mac[4];
  x = ( x << 2 ) ^ mac[5];

  x ^= x >> 8;

  ret = x & ( BR_HASH_SIZE - 1 );

  BDEBUG ( "in %02x:%02x:%02x:%02x:%02x:%02x  and Hash %d\n",
           mac[0], mac[1], mac[2], mac[3], mac[4], mac[5], ret );

  return ret;
}

/*
  This function adds an entry to the forwarding DB
  for the specified bridge.
*/

static __inline__ void
hash_link ( struct apn_bridge *br, struct apn_bridge_fdb_entry *ent, int hash )
{
  ent->next_hash = br->hash[hash];
  if ( ent->next_hash != NULL )
    ent->next_hash->pprev_hash = &ent->next_hash;
  br->hash[hash] = ent;
  ent->pprev_hash = &br->hash[hash];
}

/*
  hash_unlink removes an entry from the FDB.
*/

static __inline__ void
hash_unlink ( struct apn_bridge_fdb_entry *ent )
{
  *( ent->pprev_hash ) = ent->next_hash;
  if ( ent->next_hash != NULL )
    ent->next_hash->pprev_hash = ent->pprev_hash;
  ent->next_hash = NULL;
  ent->pprev_hash = NULL;
}

void
br_fdb_changeaddr ( struct apn_bridge_port *port, unsigned char *newaddr )
{
  register int i;
  struct apn_bridge *br = port->br;

  BR_WRITE_LOCK_BH ( &br->hash_lock );
  for ( i = 0; i < BR_HASH_SIZE; i++ )
    {
      register struct apn_bridge_fdb_entry *fdb_entry = br->hash[i];
      while ( fdb_entry != NULL )
        {
          if ( fdb_entry->port == port && fdb_entry->is_local )
            {
              hash_unlink ( fdb_entry );
              memcpy ( fdb_entry->addr.addr, newaddr, ETH_ALEN );
              hash_link ( br, fdb_entry, br_mac_hash ( newaddr ) );
              BR_WRITE_UNLOCK_BH ( &br->hash_lock );
              return;
            }
          fdb_entry = fdb_entry->next_hash;
        }
    }
  BR_WRITE_UNLOCK_BH ( &br->hash_lock );
}

/* br_fdb_cleanup finds and removes all expired dynamic entires from the dynamic fdb. */

void
br_fdb_cleanup ( struct apn_bridge *br )
{
  register int i;
  struct apn_multicast_port_list *mport, *mport_next;

  BR_WRITE_LOCK_BH ( &br->hash_lock );
  for ( i = 0; i < BR_HASH_SIZE; i++ )
    {
      register struct apn_bridge_fdb_entry *fdb_entry = br->hash[i];
      while ( fdb_entry != NULL )
        {
          struct apn_bridge_fdb_entry *next_entry = fdb_entry->next_hash;
          if ( has_expired ( br, fdb_entry ) )
            {
              if ( !memcmp ( fdb_entry->addr.addr, igmp_group_addr, 3 ) )
                {
                  mport = fdb_entry->mport;
                  while ( mport )
                    {
                      mport_next = mport->next;
                      kfree ( mport );
                      mport = mport_next;
                    }

                  br->num_dynamic_entries--;
                  hash_unlink ( fdb_entry );
                  br_fdb_put ( fdb_entry );
                }
              else
                {
                  br->num_dynamic_entries--;
                  hash_unlink ( fdb_entry );
                  br_fdb_put ( fdb_entry );
                }
            }
          fdb_entry = next_entry;
        }
    }
  BR_WRITE_UNLOCK_BH ( &br->hash_lock );
}

/* br_fdb_free_multicast_port.
   This frees a port from a multicast fdb entry. */
void
br_fdb_free_multicast_port ( struct apn_bridge *br,
                             struct apn_bridge_fdb_entry *fdb_entry,
                             struct apn_bridge_port *port )
{
  int found = 0;
  struct apn_multicast_port_list *mport, *pmport = NULL;

  mport = fdb_entry->mport;
  while ( mport )
    {
      if ( mport->port == port )
        {
          found = 1;
          break;
        }

      pmport = mport;
      mport = mport->next;
    }

  if ( found )
    {
      if ( ( mport->next == NULL ) && ( pmport == NULL ) )
        {
          /* Last entry deleted. */
          kfree ( mport );
          br->num_dynamic_entries--;
          hash_unlink ( fdb_entry );
          br_fdb_put ( fdb_entry );
        }
      else if ( pmport == NULL )
        {
          fdb_entry->mport = mport->next;
          kfree (mport);
        }
      else
        {
          pmport->next = mport->next;
          kfree ( mport );
        }
    }
}

/* br_fdb_delete_all_by_port removes all entries (static and dynamic) 
   from the fdb associated with a specified port. */

void
br_fdb_delete_all_by_port ( struct apn_bridge *br,
                            struct apn_bridge_port *port)
{
  register int i;

  /* Note: Eventhough we take the FID argument this is not actuall used 
     for flushing. In short all the entries belonging to the port are 
     deleted immaterial of the FID. Change this later */
  /* clear dynamic entries fo the port */

  BR_WRITE_LOCK_BH ( &br->hash_lock );

  for ( i = 0; i < BR_HASH_SIZE; i++ )
    {
      register struct apn_bridge_fdb_entry *fdb_entry = br->hash[i];
      while ( fdb_entry != NULL )
        {
          struct apn_bridge_fdb_entry *next_entry = fdb_entry->next_hash;
          if ( !memcmp ( fdb_entry->addr.addr, igmp_group_addr, 3 ) )
            br_fdb_free_multicast_port ( br, fdb_entry, port );
          else if ( ( fdb_entry->port == port ) )
            {
              br->num_dynamic_entries--;
              hash_unlink ( fdb_entry );
              br_fdb_put ( fdb_entry );
            }
          fdb_entry = next_entry;
        }
    }
  BR_WRITE_UNLOCK_BH ( &br->hash_lock );

  /* now clear the static FDB entries for the port */
  BR_WRITE_LOCK_BH ( &br->static_hash_lock );
  for ( i = 0; i < BR_HASH_SIZE; i++ )
    {
      register struct apn_bridge_fdb_entry *fdb_entry = br->static_hash[i];
      while ( fdb_entry != NULL )
        {
          struct apn_bridge_fdb_entry *next_entry = fdb_entry->next_hash;
          if ( !memcmp ( fdb_entry->addr.addr, igmp_group_addr, 3 ) )
            br_fdb_free_multicast_port ( br, fdb_entry, port );
          else if ( fdb_entry->port == port )
            {
              br->num_static_entries--;
              hash_unlink ( fdb_entry );
              br_fdb_put ( fdb_entry );
            }
          fdb_entry = next_entry;
        }
    }
  BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
}

extern void
br_fdb_delete_dynamic_by_vlan (struct apn_bridge * br,
                               enum vlan_type type, vid_t vid)
{
  register int i;

  /* clear dynamic entries fo the port */

  BR_WRITE_LOCK_BH ( &br->hash_lock );

  for ( i = 0; i < BR_HASH_SIZE; i++ )
    {
      register struct apn_bridge_fdb_entry *fdb_entry = br->hash[i];

      while ( fdb_entry != NULL )
        {
          struct apn_bridge_fdb_entry *next_entry = fdb_entry->next_hash;

          if ( ( (( type == CUSTOMER_VLAN ) && ( fdb_entry->vid == vid ))
                || (( type == SERVICE_VLAN ) && ( fdb_entry->svid == vid)) )
              && !fdb_entry->is_local )
            {
              BDEBUG ( "Deleting dyn entry at hash %d \n", i );
              br->num_dynamic_entries--;
              hash_unlink ( fdb_entry );
              br_fdb_put ( fdb_entry );
            }

          fdb_entry = next_entry;
        }
    }

  BR_WRITE_UNLOCK_BH ( &br->hash_lock );
}

/* br_fdb_delete_dynamic_by_port removes dynamic entries
   from the fdb associated with a specified port. It does not
   remove the local port entry. */

void
br_fdb_delete_dynamic_by_port (struct apn_bridge *br,
                               struct apn_bridge_port *port,
                               unsigned int instance, vid_t vid, vid_t svid)
{
  register int i;

  /* clear dynamic entries fo the port */

  BR_WRITE_LOCK_BH ( &br->hash_lock );
  for ( i = 0; i < BR_HASH_SIZE; i++ )
    {
      register struct apn_bridge_fdb_entry *fdb_entry = br->hash[i];
      while ( fdb_entry != NULL )
        {
          struct apn_bridge_fdb_entry *next_entry = fdb_entry->next_hash;

          if (port && vid)
            {
              if ( ( fdb_entry->port == port ) && !fdb_entry->is_local &&
                   ( fdb_entry->vid == vid ) && ( fdb_entry->svid == svid))
                {
                  BDEBUG ( "Deleting dyn entry at hash %d \n", i );
                  br->num_dynamic_entries--;
                  hash_unlink ( fdb_entry );
                  br_fdb_put ( fdb_entry );
                }
            }
          else if (port && instance)
            {
              if (( fdb_entry->port == port ) && !fdb_entry->is_local)
                {
                  struct vlan_info_entry *vlan = NULL;

                  if (br->type == BR_TYPE_PROVIDER_RSTP
                      || br->type == BR_TYPE_PROVIDER_MSTP)
                    {
                      vlan = br->svlan_info_table[fdb_entry->svid];
                    }
                  else
                    {
                      vlan = br->vlan_info_table[fdb_entry->vid];
                    }

                  if (vlan && vlan->instance == instance)
                    {
                      BDEBUG ( "Deleting dyn entry at hash %d \n", i );
                      br->num_dynamic_entries--;
                      hash_unlink ( fdb_entry );
                      br_fdb_put ( fdb_entry );
                    }
                }
            }
          else if (port)
            {
              if ( ( fdb_entry->port == port ) && !fdb_entry->is_local )
                {
                  BDEBUG ( "Deleting dyn entry at hash %d \n", i );
                  br->num_dynamic_entries--;
                  hash_unlink ( fdb_entry );
                  br_fdb_put ( fdb_entry );
                }
            }
          else if (vid)
            {
              if ( ( fdb_entry->vid == vid ) && ( fdb_entry->svid == vid)
                  && !fdb_entry->is_local )
                {
                  BDEBUG ( "Deleting dyn entry at hash %d \n", i );
                  br->num_dynamic_entries--;
                  hash_unlink ( fdb_entry );
                  br_fdb_put ( fdb_entry );
                }
            } 

          fdb_entry = next_entry;
        }
    }
  BR_WRITE_UNLOCK_BH ( &br->hash_lock );
}

/* br_fdb_get_pvlan_entry returns an fdb entry for pvlan configured port */
struct apn_bridge_fdb_entry *
br_fdb_get_pvlan_entry (struct apn_bridge *br, unsigned char *addr, vid_t vid )
{
  struct vlan_info_entry *vlan = br->vlan_info_table[vid];
  struct apn_bridge_fdb_entry *fdb_entry = NULL ;
  int secondary_vid;

  if (!vlan)
    return NULL;
  
  if (!vlan->pvlan_configured)
    return NULL;

  switch (vlan->pvlan_type)
    {
    case PVLAN_ISOLATED:
      BDEBUG (" PVLAN_ISOLATED \n");
      if (vlan->pvlan_info.vid.primary_vid == 0)
        break;
      fdb_entry = br_fdb_get (br, addr, vlan->pvlan_info.vid.primary_vid,
                              vlan->pvlan_info.vid.primary_vid);
      break;
    case PVLAN_COMMUNITY:
      BDEBUG (" PVLAN_COMMUNITY \n");
      fdb_entry = br_fdb_get (br, addr, vid, vid);
      if (fdb_entry)
        break;
      if (vlan->pvlan_info.vid.primary_vid == 0)
        break;
      fdb_entry = br_fdb_get (br, addr, vlan->pvlan_info.vid.primary_vid,
                              vlan->pvlan_info.vid.primary_vid);
      break;
    case PVLAN_PRIMARY:
      BDEBUG (" PVLAN_PRIMARY \n");
      fdb_entry = br_fdb_get (br, addr, vid, vid);
      if (fdb_entry)
        break;
      for (secondary_vid = 2; secondary_vid < VLAN_MAX_VID; secondary_vid++)
        {
          if (!BR_SERV_REQ_VLAN_IS_SET (vlan->pvlan_info.secondary_vlan_bmp,
                                        secondary_vid))
            continue;

          fdb_entry = br_fdb_get (br, addr, secondary_vid, secondary_vid);

          if (fdb_entry)
            break;
        }
      break;
    default:
      break;       
    }    

  return fdb_entry;
}

/* br_fdb_get returns an fdb entry given a bridge instance and mac address. */

struct apn_bridge_fdb_entry *
br_fdb_get ( struct apn_bridge *br, unsigned char *addr, vid_t vid, vid_t svid )
{
  register struct apn_bridge_fdb_entry *fdb_entry;

  BR_READ_LOCK_BH ( &br->hash_lock );
  fdb_entry = br->hash[br_mac_hash ( addr )];

  while ( fdb_entry != NULL )
    {
      if ( !memcmp ( fdb_entry->addr.addr, addr, ETH_ALEN ) )
        {
          if ( br->is_vlan_aware )        /* vlan-aware bridge */
            {
              /* TAKE READ LOCK */
              if ( ( fdb_entry->vid == vid ) &&
                   ( fdb_entry->svid == svid ) )
                {
                  if ( !has_expired ( br, fdb_entry ) )
                    {
                      atomic_inc ( &fdb_entry->use_count );
                      BR_READ_UNLOCK_BH ( &br->hash_lock );
                      /* RELEASE READ LOCK */
                      return fdb_entry;
                    }
                }
              /* RELEASE READ LOCK */
            }                        /* end of if vlan-aware bridge */
          else                        /* transparent bridge */
            {
              if ( !has_expired ( br, fdb_entry ) )
                {
                  atomic_inc ( &fdb_entry->use_count );
                  BR_READ_UNLOCK_BH ( &br->hash_lock );
                  return fdb_entry;
                }
            }                        /* end of if transparent bridge */

          BR_READ_UNLOCK_BH ( &br->hash_lock );
          return NULL;
        }

      fdb_entry = fdb_entry->next_hash;
    }

  BR_READ_UNLOCK_BH ( &br->hash_lock );
  return NULL;
}

/*
  br_fdb_find_and_delete removes an entry for a particular port and 
  addr if present in the dynamic FDB. If no entry is present - do nothing. 
*/

void
br_fdb_find_and_delete ( struct apn_bridge *br, unsigned char *addr,
                         struct apn_bridge_port *port )
{
  struct apn_bridge_fdb_entry *fdb;

  BR_READ_LOCK_BH ( &br->hash_lock );
  fdb = br->hash[br_mac_hash ( addr )];
  while ( fdb != NULL )
    {
      if ( !memcmp ( fdb->addr.addr, addr, ETH_ALEN )
           && !memcmp ( addr, igmp_group_addr, 3 ) )
        br_fdb_free_multicast_port ( br, fdb, port );
      else if ( ( !memcmp ( fdb->addr.addr, addr, ETH_ALEN ) )
                && ( fdb->port == port ) )
        {
          br->num_dynamic_entries--;
          hash_unlink ( fdb );
          br_fdb_put ( fdb );
          BR_READ_UNLOCK_BH ( &br->hash_lock );
          return;
        }
      fdb = fdb->next_hash;
    }

  BR_READ_UNLOCK_BH ( &br->hash_lock );
  return;
}

/* br_fdb_put deallocates an fdb entry (decrements the reference count). 
   If the entry is no longer referenced, it is deleted. */

void
br_fdb_put ( struct apn_bridge_fdb_entry *ent )
{
  if ( atomic_dec_and_test ( &ent->use_count ) )
    kfree ( ent );
}

/*
  br_fdb_get_entries is called to copy the (dynamic) FDB entries 
  into user space.
*/

int
br_fdb_get_entries ( struct apn_bridge *br,
                     unsigned char *dest_buf, int maxnum, int offset )
{
  register int i;
  int num = 0;
  struct fdb_entry *dest_ptr = ( struct fdb_entry * ) dest_buf;
  struct fdb_entry ent;
  int err;
  struct apn_bridge_fdb_entry *next_entry;
  struct apn_bridge_fdb_entry **prev_ptr;
  struct apn_multicast_port_list *mport;
  static unsigned long prev = 0;
  static unsigned long wraparound_count = 0;

  /* Check for wraparound. */
  if (prev > jiffies)
    wraparound_count++;

  prev = jiffies;

  BR_READ_LOCK_BH ( &br->hash_lock );
  for ( i = 0; i < BR_HASH_SIZE; i++ )
    {
      struct apn_bridge_fdb_entry *fdb_entry = br->hash[i];
      while ( fdb_entry != NULL && num < maxnum )
        {
          if ( has_expired ( br, fdb_entry ) )
            {
              fdb_entry = fdb_entry->next_hash;
              continue;
            }

          /* Handle starting in the middle somewhere */
          if ( offset )
            {
              offset--;
              fdb_entry = fdb_entry->next_hash;
              continue;
            }

          /* Multicast entry. */
          if ( !memcmp ( fdb_entry->addr.addr, igmp_group_addr, 3 ) )
            {
              int timer_expiry;

              memcpy ( ent.mac_addr, fdb_entry->addr.addr, ETH_ALEN );
              ent.is_local = fdb_entry->is_local;
              ent.is_fwd = fdb_entry->is_fwd;
              ent.vid = fdb_entry->vid;

              mport = fdb_entry->mport;
              while ( mport && num < maxnum )
                {
                  ent.port_no = 0;   
                  if (mport->port)
                     ent.port_no = mport->port->port_no ;
                  else
                     ent.port_no = 0;

                      timer_expiry =
                        mport->port->br->ageing_time - 
                        (((wraparound_count * WRAPAROUND_VALUE) + jiffies)
                         - fdb_entry->ageing_timestamp );
                    
                  ent.ageing_timer_value = ( fdb_entry->is_local
                                             || fdb_entry->
                                             is_static ) ? 0 : ( timer_expiry >
                                                                 0 ) ?
                    timer_expiry / HZ : 0;

                  atomic_inc ( &fdb_entry->use_count );
                  BR_READ_UNLOCK_BH ( &br->hash_lock );
                  err =
                    copy_to_user ( dest_ptr, &ent,
                                   sizeof ( struct fdb_entry ) );
                  BR_READ_LOCK_BH ( &br->hash_lock );
                  br_fdb_put ( fdb_entry );
                  if ( err )
                    {
                      BR_READ_UNLOCK_BH ( &br->hash_lock );
                      return -EFAULT;
                    }

                  num++;
                  dest_ptr++;

                  mport = mport->next;
                }
            }
          else
            {
              copy_fdb ( &ent, fdb_entry );
              atomic_inc ( &fdb_entry->use_count );
              BR_READ_UNLOCK_BH ( &br->hash_lock );
              err =
                copy_to_user ( dest_ptr, &ent, sizeof ( struct fdb_entry ) );
              BR_READ_LOCK_BH ( &br->hash_lock );
              br_fdb_put ( fdb_entry );
              if ( err )
                {
                  BR_READ_UNLOCK_BH ( &br->hash_lock );
                  return -EFAULT;
                }

              num++;
              dest_ptr++;
            }

          next_entry = fdb_entry->next_hash;
          prev_ptr = fdb_entry->pprev_hash;
          if ( next_entry == NULL && prev_ptr == NULL )
            {
              return -EAGAIN;
            }
          fdb_entry = next_entry;
        }                        /*while */
    }                                /*FOR */
  BR_READ_UNLOCK_BH ( &br->hash_lock );
  return num;
}

static __inline__ struct static_entry *
static_entry_add ( struct static_entry **fdb_list,
                   struct apn_bridge_fdb_entry *fdb_entry )
{
  struct static_entry *new_entry;
  struct apn_multicast_port_list *mport;

  if ( !memcmp ( fdb_entry->addr.addr, igmp_group_addr, 3 ) )
    {
      mport = fdb_entry->mport;
      *fdb_list = NULL;

      while ( mport )
        {
          new_entry = kmalloc ( sizeof ( struct static_entry ), GFP_ATOMIC );
          if ( new_entry )
            {
              new_entry->vid = fdb_entry->vid;
              new_entry->svid = fdb_entry->svid;
              new_entry->port = mport->port;
              new_entry->next = *fdb_list ? *fdb_list : NULL;
              *fdb_list = new_entry;
            }
          mport = mport->next;
        }
    }
  else
    {
      new_entry = kmalloc ( sizeof ( struct static_entry ), GFP_ATOMIC );
      if ( new_entry )
        {
          new_entry->vid = fdb_entry->vid;
          new_entry->svid = fdb_entry->svid;
          new_entry->port = fdb_entry->port;
          new_entry->next = *fdb_list ? *fdb_list : NULL;
          *fdb_list = new_entry;
        }
    }

  return *fdb_list;
}

static __inline__ void
static_entry_delete ( struct static_entry *forward_list )
{
  struct static_entry *current_entry, *temp;
  current_entry = forward_list;
  while ( current_entry )
    {
      temp = current_entry;
      current_entry = current_entry->next;
      kfree ( temp );
    }
}

static void
static_entries_delete ( struct static_entries *static_entries )
{
  static_entry_delete ( static_entries->forward_list );
  BR_CLEAR_PORT_MAP ( static_entries->filter_portmap );
  static_entries->is_forward = 0;
  static_entries->is_filter = 0;
}

static br_result_t
static_entries_add ( struct static_entries *static_entries,
                     struct apn_bridge_fdb_entry *curr )
{
  br_result_t retval;
  if ( curr->is_fwd )                /* forwarding entry */
    {
      if ( static_entry_add ( &static_entries->forward_list, curr ) )
        {
          static_entries->is_forward = 1;
          retval = BR_NOERROR;
        }
      else
        {
          /* Memory allocation failure */
          /* Must clear all the entries from both forward and filter */
          static_entries_delete ( static_entries );
          retval = BR_NORESOURCE;
        }
    }
  else                                /* filtering entry */
    {
      BR_SET_PMBIT ( curr->port->port_id, static_entries->filter_portmap );
      BDEBUG (" PORT_ID %d filter port map set \n", curr->port->port_id);
      static_entries->is_filter = 1;
      retval = BR_NOERROR;
    }

  return retval;
}

static void
br_fdb_static_entries_append_filter_map (struct static_entries *static_entries,
                                         struct static_entries *static_entries_append)
{
  int port_map;
  if (!static_entries_append)
    {
      BDEBUG (" static_entries_append->filter_portmap is null \n");
      return;
    }
  for (port_map = 0; port_map < BR_MAX_PORTS; port_map++)
    {
      if (BR_GET_PMBIT (port_map, static_entries_append->filter_portmap))
        {
          BR_SET_PMBIT (port_map, static_entries->filter_portmap);
          BDEBUG (" filter port map set for port_id %d \n", port_map);
          static_entries->is_filter = 1;
        }
    }
}
static void
br_fdb_static_entries_append (struct static_entries *static_entries,
                              struct static_entries *static_entries_append)
{
  struct static_entry *list =  NULL;
  int port_map;

  if (!static_entries_append)
    return;

  if (static_entries->forward_list == NULL)
    {
      BDEBUG (" static_entries->forward_list is null \n");
      static_entries->forward_list = static_entries_append->forward_list;
      if (static_entries_append->is_forward)
        static_entries->is_forward = 1;
      br_fdb_static_entries_append_filter_map (static_entries, 
                                               static_entries_append);
      return;
    }

  list = static_entries->forward_list;
  while (list != NULL)
    {
      if (list->next == NULL)
        {
          list->next = static_entries_append->forward_list;
          static_entries->is_forward = 1;
          break;
        }
      list = list->next; 
    }

  br_fdb_static_entries_append_filter_map (static_entries,
                                           static_entries_append);
}

static void
br_fdb_static_entries_print (struct static_entries *static_entries)
{
  struct static_entry *list =  NULL;

  if (static_entries->forward_list)
    list = static_entries->forward_list;

  if (list == NULL)
    {
      BDEBUG (" list is null \n");      
      return;
    }

  while (list != NULL)
    {
      BDEBUG ("list->vid = %d \n", list->vid);
      if (list->next == NULL)
        {
          BDEBUG ("list->next is null\n");
          /*list->next = static_entries_append->forward_list;*/
          break;
        }
      BDEBUG ("list = %u \n", list);
      list = list->next;
    }

}

br_result_t
br_fdb_get_static_pvlan_fdb_entries ( struct apn_bridge * br,
                                      unsigned char *addr,
                                      vid_t vid,
                                      struct static_entries * static_entries )
{
  struct vlan_info_entry *vlan = br->vlan_info_table[vid];
  struct static_entries static_entries_primary, static_entries_secondary;
  struct static_entries static_entries_local;
  vid_t secondary_vid;
  br_result_t match = BR_NOMATCH;
  /* Intialize the static_entries */
  static_entries->forward_list = NULL;
  BR_CLEAR_PORT_MAP ( static_entries->filter_portmap );
  static_entries->is_forward = 0;
  static_entries->is_filter = 0;


  if (!vlan)
    return BR_NOMATCH;

  if (!vlan->pvlan_configured)
    return match;

  switch (vlan->pvlan_type)
    {
    case PVLAN_ISOLATED:
      if ((br_fdb_get_static_fdb_entries (br, addr, vlan->pvlan_info.vid.primary_vid,
              vlan->pvlan_info.vid.primary_vid, static_entries) == BR_MATCH))
        match = BR_MATCH;
      break;

    case PVLAN_COMMUNITY:
      if (br_fdb_get_static_fdb_entries
          (br, addr, vid, vid, &static_entries_secondary) == BR_MATCH)    
        {
          br_fdb_static_entries_append (static_entries,
                                        &static_entries_secondary); 
          br_fdb_static_entries_print ( &static_entries_secondary );
 
          match = BR_MATCH;
        }
      if (vlan->pvlan_info.vid.primary_vid == 0)
        break;
      if (br_fdb_get_static_fdb_entries
          (br, addr, vlan->pvlan_info.vid.primary_vid,
           vlan->pvlan_info.vid.primary_vid,
           &static_entries_primary) == BR_MATCH)
        {
          br_fdb_static_entries_append (static_entries,
                                        &static_entries_primary); 
          BDEBUG ("static_entries \n");
          br_fdb_static_entries_print (static_entries);
          BDEBUG (" primary entries \n");
          br_fdb_static_entries_print ( &static_entries_primary);
          match = BR_MATCH;
        }
      break;

    case PVLAN_PRIMARY:
      if (br_fdb_get_static_fdb_entries
          (br, addr, vid, vid, &static_entries_primary) == BR_MATCH)
        {
          br_fdb_static_entries_append (static_entries,
                                        &static_entries_primary);
          match = BR_MATCH;
        }
      for (secondary_vid = 2; secondary_vid < VLAN_MAX_VID;
           secondary_vid++)
        {
          if (!BR_SERV_REQ_VLAN_IS_SET
              (vlan->pvlan_info.secondary_vlan_bmp, secondary_vid))
            continue;
          if (br_fdb_get_static_fdb_entries
              (br, addr, secondary_vid, secondary_vid,
               &static_entries_secondary) == BR_MATCH)
            {
              br_fdb_static_entries_append (static_entries,
                                            &static_entries_secondary);
              match = BR_MATCH;
            }
        }
      break;
    default:
      match = BR_NOMATCH;
    }

  return match;
}


br_result_t
br_fdb_get_static_fdb_entries ( struct apn_bridge * br,
                                unsigned char *addr,
                                vid_t vid,
                                vid_t svid,
                                struct static_entries * static_entries )
{
  struct apn_bridge_fdb_entry *curr;
  /* Intialize the static_entries */
  static_entries->forward_list = NULL;
  BR_CLEAR_PORT_MAP ( static_entries->filter_portmap );
  static_entries->is_forward = 0;
  static_entries->is_filter = 0;
  BR_READ_LOCK_BH ( &br->static_hash_lock );
  curr = br->static_hash[br_mac_hash ( addr )];
  if ( !curr )
    {
      BR_READ_UNLOCK_BH ( &br->static_hash_lock );
      return BR_NOMATCH;
    }

  /* Walk through the staticfdb for the given hash(addr) */
  while ( curr )
    {
      if ( !memcmp ( curr->addr.addr, addr, ETH_ALEN ) )
        {
          if ( br->is_vlan_aware )        /* vlan-aware bridge */
            {
              /* TAKE READ LOCK */
              if ( curr->vid == vid 
                   && curr->svid == svid )
                {
                  if ( static_entries_add ( static_entries, curr ) !=
                       BR_NOERROR )
                    {
                      BR_READ_UNLOCK_BH ( &br->static_hash_lock );
                      /* RELEASE READ LOCK */
                      return BR_NORESOURCE;
                    }
                }                /* end of if vids are equal */
              /* RELEASE READ LOCK */
            }                        /* end of if vlan-aware bridge */
          else                        /* transparent bridge */
            {
              if ( static_entries_add ( static_entries, curr ) != BR_NOERROR )
                {
                  BR_READ_UNLOCK_BH ( &br->static_hash_lock );
                  return BR_NORESOURCE;
                }
            }                        /* end of transparent bridge */
        }                        /* end of if addresses are the same */
      curr = curr->next_hash;
    }                                /* end of while curr is valid */

  BR_READ_UNLOCK_BH ( &br->static_hash_lock );
  return ( ( static_entries->is_filter )
           || ( static_entries->is_forward ) ) ? BR_MATCH : BR_NOMATCH;
}

static void
fdb_timestamp_refresh ( struct apn_bridge_fdb_entry *fdb,
                        struct apn_bridge_port *source )
{
  struct apn_multicast_port_list *mport, *mport_new;
  if ( !memcmp ( fdb->addr.addr, igmp_group_addr, 3 ) )
    {
      /* Check if the port exists. */
      mport = fdb->mport;
      while ( mport )
        {
          if ( mport->port == source )
            {
              fdb->ageing_timestamp = jiffies;
              return;
            }

          /* If last element, break. */
          if ( !mport->next )
            break;
          mport = mport->next;
        }

      /* This is a new entry. Allocate and add to the list. */
      mport_new =
        kmalloc ( sizeof ( struct apn_multicast_port_list ), GFP_ATOMIC );
      if ( !mport_new )
        return;                        /* TODO failure of adding a entry should be handled.
                                   Ideally this should be broadcast for the worst case. */
      mport->next = mport_new;
      mport_new->port = source;
      mport_new->next = NULL;

      fdb->ageing_timestamp = jiffies;
      return;
    }
  else
    {
      fdb->port = source;
      fdb->ageing_timestamp = jiffies;
      return;
    }
}

/*
  This function adds an entry to the dynamic forwarding database
  unless it already exists in the static FDB.
*/

int
br_fdb_insert ( struct apn_bridge *br,
                struct apn_bridge_port *port,
                unsigned char *addr, vid_t vid, vid_t svid,
                int is_local, bool_t is_forward)
{
  int hash = br_mac_hash ( addr );
#ifdef HAVE_MAC_AUTH
  struct auth_port *auth_port;
  auth_port = NULL;
#endif
  /* Check in the static fdb for an entry. Should there be one already present,
     do not add a dynamic entry. */
  /* Note: The check for an entry in static fdb will be based on the following
     transparent bridge: key <dest MAC, ingress port>
     vlan-aware bridge : key <dest MAC, ingress port, FID> */
  struct apn_bridge_fdb_entry *fdb_entry = br_fdb_get_static ( br, addr, port,
                                                               vid, svid );
  struct apn_multicast_port_list *mport;

  BDEBUG
    ( "in dynamic Add %02x:%02x:%02x:%02x:%02x:%02x and Hash %d\n",
      addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], hash );

  if ( fdb_entry )
    {
      /* Static entry present for this port */
      return BR_MATCH;
    }

  /* check if the fdb exceeds limits */
  if ( br->num_dynamic_entries >= BR_MAX_DYNAMIC_ENTRIES )
    {
      /* We can remove an existing entry to make space that is for later */
      return BR_NORESOURCE;
    }

  /* check if the user passed svid and vid are consistent with
     the vlan_info_table information */

#ifdef HAVE_MAC_AUTH
  if ((port != NULL) && (port->dev != NULL))
    auth_port = auth_find_port (port->dev->ifindex);
#endif

  BR_WRITE_LOCK_BH ( &br->hash_lock );

  /* Check the dynamic fdb for an entry. Should an entry exists update the time
     stamp and return */
  /* Note: The check for an entry in dynamic fdb will be based on the following 
     transparent bridge: key <dest MAC>
     vlan-aware bridge : key <dest MAC, FID> */
  fdb_entry = br->hash[hash];
  while ( fdb_entry != NULL )
    {
      if ( !fdb_entry->is_local &&
           !memcmp ( fdb_entry->addr.addr, addr, ETH_ALEN ) )
        {
          if ( br->is_vlan_aware )
            {
              if ( fdb_entry->vid == vid && fdb_entry->svid == svid)
                {
#ifdef HAVE_MAC_AUTH
                  if ((auth_port == NULL)
                      || (auth_port->auth_state != MACAUTH_ENABLED))
#endif
                    {
                      fdb_timestamp_refresh ( fdb_entry, port );
                      BR_WRITE_UNLOCK_BH ( &br->hash_lock );
                      return BR_MATCH;
                    }
                }
            }
          else
            {
#ifdef HAVE_MAC_AUTH
              if ((auth_port == NULL)
                  || (auth_port->auth_state != MACAUTH_ENABLED))
#endif
                {
                  fdb_timestamp_refresh ( fdb_entry, port );
                  BR_WRITE_UNLOCK_BH ( &br->hash_lock );
                  return BR_MATCH;
                }
            }
        }

      fdb_entry = fdb_entry->next_hash;
    }

  /* New dynamic fdb entry */
  fdb_entry = kmalloc ( sizeof (struct apn_bridge_fdb_entry), GFP_ATOMIC );
  if ( fdb_entry == NULL )
    {
      BR_WRITE_UNLOCK_BH ( &br->hash_lock );
      return BR_NORESOURCE;
    }

  memcpy ( fdb_entry->addr.addr, addr, ETH_ALEN );
  atomic_set ( &fdb_entry->use_count, 1 );
  fdb_entry->port = port;
  fdb_entry->is_local = is_local;
  fdb_entry->is_static = is_local;        /* Local addresses flagged as static. */
  fdb_entry->is_fwd = is_forward;
  fdb_entry->ageing_timestamp = jiffies;
  if ( !memcmp ( fdb_entry->addr.addr, igmp_group_addr, 3 ) )
    {
      /* Multicast. */
      mport = kmalloc ( sizeof ( struct apn_multicast_port_list ), GFP_ATOMIC );
      if ( !mport )
        {
          BR_WRITE_UNLOCK_BH ( &br->hash_lock );
          kfree ( fdb_entry );
          return BR_NORESOURCE;
        }

      mport->port = port;
      mport->next = NULL;
      fdb_entry->mport = mport;
      /* Set unicast port to NULL. */
      fdb_entry->port = NULL;
    }
  else
    fdb_entry->port = port;

  if ( br->is_vlan_aware )
    {
      fdb_entry->vid  = vid;
      fdb_entry->svid = svid;
    }
  else
    {
      fdb_entry->vid = VLAN_NULL_VID;
      fdb_entry->svid = VLAN_NULL_FID;
    }

  br->num_dynamic_entries++;
  hash_link ( br, fdb_entry, hash );
  BDEBUG
    ( "Inserting an entry with vid(%u), svid(%u)\n",
      fdb_entry->vid, fdb_entry->svid );
  BR_WRITE_UNLOCK_BH ( &br->hash_lock );
  return BR_NOERROR;
}

int
br_fdb_delete ( struct apn_bridge *br, struct apn_bridge_port *port, 
                unsigned char *addr, vid_t vid, vid_t svid )
{
  struct apn_bridge_fdb_entry *fdb_entry = 0;
  /* Take the lock */
  BR_WRITE_LOCK_BH ( &br->hash_lock );
  fdb_entry = br->hash[br_mac_hash ( addr )];
  while ( fdb_entry != NULL )
    {
      if ( ( !memcmp
             ( fdb_entry->addr.addr, addr,
               ETH_ALEN ) )
           && ( fdb_entry->vid == vid ) && ( fdb_entry->svid == svid) )
        {
          if ( !memcmp ( fdb_entry->addr.addr, igmp_group_addr, 3 ) )
            br_fdb_free_multicast_port ( br, fdb_entry, port );
          else
            {
              BDEBUG
                ( "Deleting an entry with mac (%0x:%0x:%0x:%0x:%0x:%0x) "
                  "vid(%u), svid(%u)\n", addr[0],
                  addr[1], addr[2], addr[3], addr[4],
                  addr[5], fdb_entry->vid, fdb_entry->svid );
              br->num_dynamic_entries--;
              hash_unlink ( fdb_entry );
              br_fdb_put ( fdb_entry );
              break;
            }
        }

      fdb_entry = fdb_entry->next_hash;
    }

  /* Release the lock */
  BR_WRITE_UNLOCK_BH ( &br->hash_lock );
  return BR_NOERROR;
}

void
br_clear_dynamic_fdb_by_mac ( struct apn_bridge *br,  unsigned char *addr)
{
  struct apn_bridge_fdb_entry *fdb_entry = 0;
  struct apn_bridge_fdb_entry *next_entry = 0;

  if ( !br || !addr)
    return;

  BR_WRITE_LOCK_BH ( &br->hash_lock );
  fdb_entry = br->hash[br_mac_hash ( addr )];
  while ( fdb_entry != NULL )
    {
      next_entry = fdb_entry->next_hash;
      if ( !memcmp ( fdb_entry->addr.addr, addr, ETH_ALEN ))
        {
          BDEBUG ( "Deleting an entry with mac (%0x:%0x:%0x:%0x:%0x:%0x) "
                   "vid(%u), svid(%u)\n", addr[0],
                   addr[1], addr[2], addr[3], addr[4],
                   addr[5], fdb_entry->vid, fdb_entry->svid );
          br->num_dynamic_entries--;
          hash_unlink ( fdb_entry );
          br_fdb_put ( fdb_entry );
        }

      fdb_entry = next_entry;
    }

  BR_WRITE_UNLOCK_BH ( &br->hash_lock );
  return;
}

/* ---------------------------------------------------------------*/

/* 
   This function links an fdb entry into the static hash table.
*/

static __inline__ void
static_hash_link ( struct apn_bridge
                   *br, struct apn_bridge_fdb_entry *ent, int hash )
{
  ent->next_hash = br->static_hash[hash];
  if ( ent->next_hash != NULL )
    ent->next_hash->pprev_hash = &ent->next_hash;
  br->static_hash[hash] = ent;
  ent->pprev_hash = &br->static_hash[hash];
}

/* 
   This function allocates and initializes an fdb_entry for the
   static hash table.

*/

static __inline__ struct apn_bridge_fdb_entry *
create_static_fdb_entry ( struct apn_bridge_port *source, unsigned char *addr,
                          int is_fwd, vid_t vid, vid_t svid)
{
  struct apn_bridge_fdb_entry *fdb = kmalloc ( sizeof ( *fdb ), GFP_ATOMIC );
  struct apn_multicast_port_list *mport;

  if ( fdb )
    {
      memcpy ( fdb->addr.addr, addr, ETH_ALEN );
      atomic_set ( &fdb->use_count, 1 );
      if ( !memcmp ( addr, igmp_group_addr, 3 ) )
        {
          mport =
            kmalloc ( sizeof ( struct apn_multicast_port_list ), GFP_ATOMIC );
          if ( !mport )
            {
              kfree ( fdb );
              return NULL;
            }
          fdb->mport = mport;
          fdb->mport->port = source;
          fdb->mport->next = NULL;
        }
      else
        {
          fdb->port = source;
        }

      fdb->is_local = 0;
      fdb->is_fwd = is_fwd;
      fdb->is_static = 1;
      fdb->ageing_timestamp = 0;
      fdb->svid = svid;
      fdb->vid = vid;
    }
  return fdb;
}

/* Add an entry in the static database
   Make sure same mac addresses are linked together
   primary key is mac addr and port
*/
int
br_fdb_insert_static ( struct apn_bridge *br, struct apn_bridge_port *source,
                       unsigned char *addr, int is_fwd, vid_t vid, vid_t svid )
{
  struct apn_bridge_fdb_entry *fdb;
  int hash = br_mac_hash ( addr );
  struct apn_multicast_port_list *mport, *mport_new;
  int addr_present = 0;

  BDEBUG ( "in Static  Add %02x:%02x:%02x:%02x:%02x:%02x  and Hash %d\n",
           addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], hash );
  if ( br->num_static_entries >= BR_MAX_STATIC_ENTRIES )
    return -EMFILE;
  BR_WRITE_LOCK_BH ( &br->static_hash_lock );
  fdb = br->static_hash[hash];

  while ( fdb != NULL )
    {
      addr_present = ( (!memcmp ( fdb->addr.addr, addr, ETH_ALEN ))
                       && (fdb->vid == vid) && (fdb->svid == svid));

      BDEBUG ( "ADDR PRESENT is %d \n", addr_present );

      if ( addr_present )
        {
          if ( !memcmp ( addr, igmp_group_addr, 3 ) )
            {
              /* Check if the port exists. */
              mport = fdb->mport;
              while ( mport )
                {
                  if ( mport->port == source )
                    {
                      /* found address and port match */
                      BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
                      return -EEXIST;
                    }

                  /* If last element, break. */
                  if ( !mport->next )
                    break;
                  mport = mport->next;
                }

              /* This is a new entry. Allocate and add to the list. */
              mport_new =
                kmalloc ( sizeof ( struct apn_multicast_port_list ),
                          GFP_ATOMIC );
              if ( !mport_new )
                {
                  BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
                  return -EFAULT;
                }
              mport->next = mport_new;
              mport_new->port = source;
              mport_new->next = NULL;
              BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
              return 0;
            }
          else
            {
              if ( fdb->port == source )
                {
                  /* found address and port match */
                  /* Check if we're changing the entry from forward 
                     to discard or viz. */
                  if (fdb->is_fwd != is_fwd)
                    {
                      /* Change it */
                      fdb->is_fwd = is_fwd;
                      BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
                      return 0;
                    }
                  BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
                  return -EEXIST;
                }
              else
                {
                  /* Port parameter did not match, insert 
                     new entry in list near matched entry. */
                  struct apn_bridge_fdb_entry *new_entry =
                    create_static_fdb_entry ( source, addr,
                                              is_fwd, vid, svid);
                  if ( new_entry == NULL )
                    {
                      BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
                      return -EFAULT;
                    }

                  /* link new entry appropriately */
                  br->num_static_entries++;
                  static_hash_link ( br, new_entry, hash );
                  BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
                  return 0;
                }
            }
        }                        /*else if */
      /* continue looking */
      fdb = fdb->next_hash;
    }                                /* while */

  /* No address match, so append new entry to the end of the list */
  fdb = create_static_fdb_entry ( source, addr, is_fwd, vid, svid );
  br->num_static_entries++;
  static_hash_link ( br, fdb, hash );
  BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
  return 0;
}

int
br_fdb_rem_static ( struct apn_bridge *br, struct apn_bridge_port *source,
                    unsigned char *addr, vid_t vid, vid_t svid )
{
  struct apn_bridge_fdb_entry *fdb;
  int hash = br_mac_hash ( addr );
  BR_WRITE_LOCK_BH ( &br->static_hash_lock );
  fdb = br->static_hash[hash];
  while ( fdb != NULL )
    {
      if ( ( !memcmp ( fdb->addr.addr, addr, ETH_ALEN ) )
           && ( fdb->vid == vid ) && ( fdb->svid == svid) )
        {
          if ( !memcmp ( fdb->addr.addr, igmp_group_addr, 3 ) )
            {
              br_fdb_free_multicast_port ( br, fdb, source );
              BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
              return 0;
            }
          else if (fdb->port == source)
            {
              br->num_static_entries--;
              hash_unlink ( fdb );
              br_fdb_put ( fdb );
              BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
              return 0;
            }
        }
      fdb = fdb->next_hash;
    }
  BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
  return 0;
}

struct apn_bridge_fdb_entry *
br_fdb_get_static ( struct apn_bridge *br,
                    unsigned char *addr,
                    struct apn_bridge_port *port, vid_t vid, vid_t svid)
{
  struct apn_bridge_fdb_entry *fdb;
  int found = 0;
  struct apn_multicast_port_list *mport;

  BR_READ_LOCK_BH ( &br->static_hash_lock );

  fdb = br->static_hash[br_mac_hash ( addr )];

  while ( fdb != NULL )
    {
      if ( !memcmp ( fdb->addr.addr, addr, ETH_ALEN ) && ( fdb->port == port ) )
        {
          if ( !memcmp ( fdb->addr.addr, igmp_group_addr, 3 ) )
            {
              mport = fdb->mport;
              while ( mport )
                {
                  if ( mport->port == port )
                    {
                      found = 1;
                      break;
                    }

                  mport = mport->next;
                }
            }
          else if ( fdb->port == port )
            {
              found = 1;
            }

          if ( !found )
            {
              BR_READ_UNLOCK_BH ( &br->static_hash_lock );
              return NULL;
            }

          if ( br->is_vlan_aware )
            {
              /* TAKE READ LOCK */
              if ( fdb->vid == vid && fdb->svid == svid )
                {
                  BR_READ_UNLOCK_BH ( &br->static_hash_lock );
                  /* RELEASE READ LOCK */
                  return fdb;
                }
              /* RELEASE READ LOCK */
            }
          else
            {
              BR_READ_UNLOCK_BH ( &br->static_hash_lock );
              return fdb;
            }
        }

      fdb = fdb->next_hash;
    }

  BR_READ_UNLOCK_BH ( &br->static_hash_lock );
  return NULL;
}

int
br_fdb_get_static_entries ( struct apn_bridge *br,
                            unsigned char *_buf, int maxnum, int offset )
{
  register int i;
  int num = 0;
  struct fdb_entry *user_buf = ( struct fdb_entry * ) _buf;
  struct apn_multicast_port_list *mport;
  static unsigned long prev = 0;
  static unsigned long wraparound_count = 0;

  /* Check for wraparound. */
  if (prev > jiffies)
    wraparound_count++;

  prev = jiffies;

  BR_READ_LOCK_BH ( &br->static_hash_lock );
  for ( i = 0; i < BR_HASH_SIZE; i++ )
    {
      struct apn_bridge_fdb_entry *current_entry = br->static_hash[i];
      while ( current_entry != NULL && num < maxnum )
        {
          struct fdb_entry ent;
          int err;
          struct apn_bridge_fdb_entry *next_entry;
          struct apn_bridge_fdb_entry **prev_entry;
          if ( offset )
            {
              offset--;
              current_entry = current_entry->next_hash;
              continue;
            }

          if ( !memcmp ( current_entry->addr.addr, igmp_group_addr, 3 ) )
            {
              int timer_expiry = 0;

              memcpy ( ent.mac_addr, current_entry->addr.addr, ETH_ALEN );
              ent.is_local = current_entry->is_local;
              ent.is_fwd = current_entry->is_fwd;
              ent.vid = current_entry->vid;

              mport = current_entry->mport;
              while ( mport && num < maxnum )
                {
                  if (mport->port)
                    {
                      ent.port_no = mport->port ? mport->port->port_no : 0;

                      timer_expiry =
                        mport->port->br->ageing_time  
                        - (((wraparound_count * WRAPAROUND_VALUE) + jiffies)
                            - current_entry->ageing_timestamp );
                    }
                  ent.ageing_timer_value = ( current_entry->is_local
                                             || current_entry->
                                             is_static ) ? 0 : ( timer_expiry >
                                                                 0 ) ?
                    timer_expiry / HZ : 0;

                  BR_READ_UNLOCK_BH ( &br->static_hash_lock );
                  err =
                    copy_to_user ( user_buf, &ent,
                                   sizeof ( struct fdb_entry ) );
                  BR_READ_LOCK_BH ( &br->static_hash_lock );
                  if ( err )
                    {
                      BR_READ_UNLOCK_BH ( &br->static_hash_lock );
                      return -EFAULT;
                    }

                  num++;
                  user_buf++;

                  mport = mport->next;
                }
            }
          else
            {
              copy_fdb ( &ent, current_entry );
              BR_READ_UNLOCK_BH ( &br->static_hash_lock );
              err =
                copy_to_user ( user_buf, &ent, sizeof ( struct fdb_entry ) );
              BR_READ_LOCK_BH ( &br->static_hash_lock );
              num++;
              user_buf++;
              if ( err )
                {
                  BR_READ_UNLOCK_BH ( &br->static_hash_lock );
                  return -EFAULT;
                }

            }
          next_entry = current_entry->next_hash;
          prev_entry = current_entry->pprev_hash;

          if ( next_entry == NULL && prev_entry == NULL )
            {
              BR_READ_UNLOCK_BH ( &br->static_hash_lock );
              return -EAGAIN;
            }

          current_entry = next_entry;
        }                        /* while */
    }                                /* FOR */
  BR_READ_UNLOCK_BH ( &br->static_hash_lock );
  return num;
}

struct apn_bridge_fdb_entry *
br_get_curr_fdb (struct apn_bridge_fdb_entry *start, char *mac_addr, unsigned short vid,
                 bool_t is_vlan_aware)
{

  struct apn_bridge_fdb_entry *curr = start;

  while (curr)
    {
      if ( !memcmp ( curr->addr.addr, mac_addr, ETH_ALEN ) )
        {
          if ( is_vlan_aware )
            {
              if ( curr->vid == vid 
                   && curr->svid == vid )
                {
                  return curr;
                }
            }
          else
            {
              return curr;
            }
        }
      curr = curr->next_hash;
    }

  return NULL;

}

int br_get_fdb_range ( struct apn_bridge *br, unsigned char *mac_addr, unsigned short vid,
                       int maxnum, unsigned char *dest_buf, int type)
{

  register int i;
  int num = 0, ret = 0;
  struct fdb_entry *dest_ptr = ( struct fdb_entry * ) dest_buf;
  struct fdb_entry ent;
  struct apn_bridge_fdb_entry *start, *curr;
  static unsigned long prev = 0;
  static unsigned long wraparound_count = 0;

  /* Check for wraparound. */
  if (prev > jiffies)
    wraparound_count++;

  prev = jiffies;

  if ( mac_addr == NULL )
    {
      i = 0;
      curr = br->hash[i];
    }
  else
    {
      i = br_mac_hash ( mac_addr );
      curr = br_get_curr_fdb (br->hash[i], mac_addr, vid, br->is_vlan_aware);
      /* found the entry corresponding to the mac address passed, start from
       * next entry
       */

      if ( curr )
        curr = curr->next_hash;
    } 

  BR_READ_LOCK_BH ( &br->hash_lock );
  num = br_copy_fdb_range_to_usr (br, curr, dest_buf, type, i, maxnum, wraparound_count);
  BR_READ_UNLOCK_BH ( &br->hash_lock );

  return num;
}

int
br_copy_fdb_range_to_usr ( struct apn_bridge *br,
                           struct apn_bridge_fdb_entry *start_fdb, 
                           unsigned char *dest_buf, int type, 
                           int start_hash, int maxnum, int wraparound_count)
{
  struct hal_fdb_entry *user_buf = ( struct hal_fdb_entry * ) dest_buf;
  struct apn_bridge_fdb_entry *current_entry = start_fdb;
  struct apn_multicast_port_list *mport;
  register int i;
  int num = 0;

  BDEBUG ( " Bridge br %u start_fdb %u dest_buf %u \n", br,
           start_fdb, dest_buf);

  BDEBUG ( " type %u start_hash %u maxnum %u wraparound_count %u \n", 
           type, start_hash, maxnum, wraparound_count);

  for ( i = start_hash; i < BR_HASH_SIZE; i++ )
    {

      /* For the start has start from start_fdb. For the other hash
       * queues start from the beginning.
       */  

      if ( i != start_hash) 
        current_entry = br->hash[i];

      while ( current_entry != NULL && num < maxnum )
        {
          struct hal_fdb_entry ent;
          int err;
          struct apn_bridge_fdb_entry *next_entry;
          struct apn_bridge_fdb_entry **prev_entry;

          /* Skip the entry if it has expired or requested type is multicast and
           * the current entry is unicast or thw requested type is unicast and
           * the current entry is multicast
           */

          if ( has_expired ( br, current_entry)  || 
               ( (type == BR_UNICAST_FDB) && 
                  (current_entry->addr.addr[0] & 1) ) ||
               ( (type == BR_MULTICAST_FDB) && 
                  !(current_entry->addr.addr[0] & 1) ) )
            {
              current_entry = current_entry->next_hash;
              continue;
            }

          if ( !memcmp ( current_entry->addr.addr, igmp_group_addr, 3 ) )
            {
              int timer_expiry;

              memcpy ( ent.mac_addr, current_entry->addr.addr, ETH_ALEN );
              ent.is_forward = current_entry->is_fwd;
              ent.vid        = current_entry->vid;

              mport = current_entry->mport;
              while ( mport && num < maxnum )
                {
                  ent.port = 0;
                 if ( mport->port)
                      ent.port =  mport->port->port_no;
                  else
                      ent.port = 0;

                      timer_expiry =
                        mport->port->br->ageing_time - 
                        (((wraparound_count * WRAPAROUND_VALUE)
                          + jiffies)
                         - current_entry->ageing_timestamp);
                    
                  ent.ageing_timer_value = ( current_entry->is_local
                                             || current_entry->is_static ) ?
                    0 : ( timer_expiry > 0 ) ?
                    timer_expiry / HZ : 0;

                  BR_READ_UNLOCK_BH ( &br->hash_lock );

                  err =
                    copy_to_user ( user_buf, &ent,
                                   sizeof ( struct hal_fdb_entry ) );

                  BR_READ_LOCK_BH ( &br->hash_lock );

                  if ( err )
                    {
                      return -EFAULT;
                    }

                  num++;
                  user_buf++;

                  mport = mport->next;
                }
            }
          else
            {
              copy_hal_fdb ( &ent, current_entry );

              BR_READ_UNLOCK_BH ( &br->hash_lock );

              err =
               copy_to_user ( user_buf, &ent, sizeof ( struct hal_fdb_entry ) );

              BR_READ_LOCK_BH ( &br->hash_lock );

              num++;
              user_buf++;
              if ( err )
                {
                  return -EFAULT;
                }

            }
          next_entry = current_entry->next_hash;
          prev_entry = current_entry->pprev_hash;

          if ( next_entry == NULL && prev_entry == NULL )
            {
              return -EAGAIN;
            }

          current_entry = next_entry;
        }                       /* while */


    }                           /* FOR */

  return num;

}

int
br_fdb_get_index_by_mac_vid (struct apn_bridge *br, unsigned char *addr,
                             vid_t vid, vid_t svid, int *index)
{
  int hash = br_mac_hash ( addr );

  *index = 0;

  /* Check in the static fdb for an entry. Should there be one already present,
     do not add a dynamic entry. */
  /* Note: The check for an entry in static fdb will be based on the following
     transparent bridge: key <dest MAC, ingress port>
     vlan-aware bridge : key <dest MAC, ingress port, FID> */

  struct apn_bridge_fdb_entry *fdb_entry = NULL;
  struct apn_multicast_port_list *mport;
  
  BDEBUG
    ( "in dynamic Add %02x:%02x:%02x:%02x:%02x:%02x and Hash %d\n",
      addr[0], addr[1], addr[2], addr[3], addr[4], addr[5], hash );

  /* check if the fdb exceeds limits */
  if ( br->num_dynamic_entries >= BR_MAX_DYNAMIC_ENTRIES )
    {
      /* We can remove an existing entry to make space that is for later */
      return BR_NORESOURCE;
    }

  BR_WRITE_LOCK_BH ( &br->hash_lock );

  /* Check the dynamic fdb for an entry. Should an entry exists update the time
     stamp and return */
  /* Note: The check for an entry in dynamic fdb will be based on the following
     transparent bridge: key <dest MAC>
     vlan-aware bridge : key <dest MAC, FID> */

  fdb_entry = br->hash[hash];

    while ( fdb_entry != NULL )
    {
      BDEBUG
        ( "Inside while loop vid(%u), svid(%u)\n",
          fdb_entry->vid, fdb_entry->svid );
      if ( !memcmp ( fdb_entry->addr.addr, addr, ETH_ALEN ) )
        {
          if ( br->is_vlan_aware )
            {
              if ( fdb_entry->vid == vid && fdb_entry->svid == svid )
                {
                  *index = fdb_entry->port->port_no;
                  BR_WRITE_UNLOCK_BH ( &br->hash_lock );
                  return BR_MATCH;
                }
            }
          else
            {
              *index = 0;
              BR_WRITE_UNLOCK_BH ( &br->hash_lock );
              return BR_MATCH;
            }
        }

      fdb_entry = fdb_entry->next_hash;
    }

  BR_WRITE_UNLOCK_BH ( &br->hash_lock );

  BR_WRITE_LOCK_BH ( &br->static_hash_lock );
  /* Check the static fdb for an entry. Should an entry exists update the time
     stamp and return */
  /* Note: The check for an entry in static fdb will be based on the following
     transparent bridge: key <dest MAC>
     vlan-aware bridge : key <dest MAC, FID> */

  fdb_entry = br->static_hash[hash];

    while ( fdb_entry != NULL )
    {
      BDEBUG
        ( "Inside while loop vid(%u), svid(%u)\n",
          fdb_entry->vid, fdb_entry->svid );
      if ( !memcmp ( fdb_entry->addr.addr, addr, ETH_ALEN ) )
        {
          if ( br->is_vlan_aware )
            {
              if ( fdb_entry->vid == vid && fdb_entry->svid == svid )
                {
                  *index = fdb_entry->port->port_no;
                  BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
                  return BR_MATCH;
                }
            }
          else
            {
              *index = 0;
              BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );
              return BR_MATCH;
            }
        }

      fdb_entry = fdb_entry->next_hash;
    }

  BR_WRITE_UNLOCK_BH ( &br->static_hash_lock );

  return BR_NOERROR;

}
