/* Copyright (C) 2001-2003  All Rights Reserved.  */

#include "mpls_fwd.h"

#include "mpls_common.h"
#include "mpls_client.h"

#include "mpls_fib.h"
#include "mpls_table.h"
#include "mpls_hash.h"
#include "forwarder.h"
#include "fncdef.h"

extern struct dst_ops labelled_dst_ops;
extern struct fib_handle *fib_handle;
extern struct mpls_hash if_hash;

struct rt_node *rt_head = NULL;
struct timer_list rt_cleanup_timer;
rwlock_t rt_tbl_lock = RW_LOCK_UNLOCKED;

struct dst_ops *dst_ops;
#if 0
int (*mpls_dst_input) (struct sk_buff *) = NULL;
#endif

const char *prot_to_str [] =
{
  "All Protocols",
  "Network Services Module",
  "Routing Information Protocol",
  "Routing Information Protocol (Next Generation)",
  "Open Shortest Path First",
  "Open Shortest Path First (Next Generation)",
  "Border Gateway Protocol",
  "Label Distribution Protocol",
  "Resource Reservation Protocol",
  "IS-IS",
  "Platform Independent Multicast Dense Mode",
  "Platform Independent Multicast Sparse Mode",
  "Virtual Terminal Shell",
  "PIM Packet Generator",
  "Bidirectional Forwarding Detection"
};

/* Free the memory used up by the ILM table.  */
void
mpls_destroy_ilm (struct mpls_table *table, uint8 protocol)
{
  struct mpls_table_node *rn;
  struct ilm_entry *ilmp;

  if (! table)
    return;

  PRINTK_DEBUG ("Cleaning up ILM table with identifier %d for protocol %s\n",
                table->ident, prot_to_str [(int) protocol]);
  
  for (rn = mpls_table_top (table); rn; rn = mpls_table_next (rn))
    {
      /* Only move forward for a valid ILM entry */
      if (! rn->info)
        continue;
      
      /* Get the ilm pointer from the route node */
      ilmp = rn->info;

      /* Make sure the protocol matches */
      if ((ilmp->protocol == protocol) || (protocol == APN_PROTO_UNSPEC))
        {
          if (ilmp->nhlfe_list)
            {
              dst_release(&ilmp->nhlfe_list->rt->u.dst);
              rt_free (ilmp->nhlfe_list->rt);
              ilmp->nhlfe_list->rt = NULL;
              kfree(ilmp->nhlfe_list->primitive);
            }
          
          /* Free ilmp memory */
          kfree(ilmp->nhlfe_list);
          kfree(ilmp);
          
          /* Set info to NULL */
          rn->info = NULL;
          
          /* Only unlock once. mpls_table_next will unlock it again
             and hence it'll be deleted */
          mpls_table_unlock_node (rn);
        }
    }
}

/* Free the memory used up by the FTN/VRF table.  */
void
mpls_destroy_ftn (struct mpls_table *table, uint8 protocol)
{
  struct mpls_table_node *rn;
  struct ftn_entry *ftnp;

  if (! table)
    return;

  PRINTK_DEBUG ("Cleaning up FTN table with identifier %d for protocol %s\n",
                table->ident,
                prot_to_str [(int) protocol]);

  for (rn = mpls_table_top (table); rn; rn = mpls_table_next (rn))
    {
      /* Only move forward for a valid ILM entry */
      if (! rn->info)
        continue;
      
      /* Get the ilm pointer from the route node */
      ftnp = rn->info;

      /* Make sure the protocol matches */
      if ((ftnp->protocol == protocol) || (protocol == APN_PROTO_UNSPEC))
        {
          if (ftnp->nhlfe_list)
            {
              dst_release(&ftnp->nhlfe_list->rt->u.dst);
              rt_free (ftnp->nhlfe_list->rt);
              ftnp->nhlfe_list->rt = NULL;
              kfree(ftnp->nhlfe_list->primitive);
            }
          
          /* Free ftnp memory */
          kfree(ftnp->nhlfe_list);
          kfree(ftnp);
          
          /* Set info to NULL */
          rn->info = NULL;

          /* Only unlock once. mpls_table_next will unlock it again
             and hence it'll be deleted */
          mpls_table_unlock_node (rn);
        }
    }
}


/*
 * The following routine initializes the global fib_handle
 * to be used by all.
 */
int
mpls_initialize_fib_handle ()
{
  PRINTK_DEBUG ("Initializing FIB handle\n");

  /* Reserve space for fib_handle */
  fib_handle = (struct fib_handle *) kmalloc (sizeof (struct fib_handle),
                                              GFP_ATOMIC);
  memset (fib_handle, 0, sizeof (struct fib_handle));
  if(fib_handle == NULL)
    {
      PRINTK_ERR ("Panic !! Could not initialize MPLS FIB\n");
      return FAILURE;
    }

  /* Set reference count to zero */
  fib_handle->refcount = 0;

  /* Allocate space for ftn table */
  fib_handle->ftn = mpls_table_init (GLOBAL_TABLE);
  if (fib_handle->ftn == NULL)
    {
      PRINTK_ERR ("Panic !! Could not initialize MPLS FIB entities\n");
      return FAILURE;
    }
  PRINTK_DEBUG ("GLOBAL FTN table created\n");
  
  /* Allocate space for Interface hash table */
  mpls_hash_init (mpls_if_key, mpls_if_cmp);

  PRINTK_DEBUG ("Hash table fired up.\n");

  /* Set the vrf list pointer to NULL. */
  fib_handle->vrf_list = NULL;

  /* Set the ILM list pointer to NULL. */
  fib_handle->ilm_list = NULL;

  return SUCCESS;
}

/*
 * The following routine destroys the global fib_handle
 * being used by all.
 */
void
mpls_destroy_fib_handle ()
{
  PRINTK_DEBUG ("Destroying FIB handle\n");

  /* Free FTN. */
  if (fib_handle->ftn)
    {
      mpls_destroy_ftn (fib_handle->ftn, APN_PROTO_UNSPEC);
      mpls_table_finish (fib_handle->ftn);
    }

  /* Free ILM tables. */
  mpls_ilm_delete_all (APN_PROTO_UNSPEC);

  /* Free Interface hash table. */
  mpls_hash_free_all ((void (*) (void *)) mpls_if_delete_only);

  /* Remove all vrf tables. */
  mpls_vrf_delete_all (fib_handle->vrf_list);

  /* Free FIB handle. */
  kfree (fib_handle);
  fib_handle = NULL;

  /* Done */
}

/*
 * Make sure that the MPLS module is not being used by anyone.
 */
void
mpls_cleanup_ok ()
{
  int i;
  int in_use = 0;

  /*
   * Make sure none of the protocols have their bit set
   * in the refcount unsigned char.
   */
  
  if (fib_handle->refcount == 0)
    return;

  for (i = 1; i < APN_PROTO_MAX; i++)
    {
      if (fib_handle->refcount & (1 << i))
        {
          PRINTK_WARNING ("MPLS Forwarder might have entries "
                          "installed by %s\n", prot_to_str [i]);
          in_use++;
        }
    }
  
  if (in_use > 0)
    {
      PRINTK_WARNING ("Please make sure that no application(s) using "
                      "the MPLS Forwarder is/are running\n");
    }
}

/*
 * Set refcount for protocol.
 */
void
set_mpls_refcount (uint8 protocol)
{
  /* Set the bit for this protocol in refcount. */
  fib_handle->refcount = fib_handle->refcount | (1 << protocol);
}

/*
 * Unset refcount for protocol.
 */
void
unset_mpls_refcount (uint8 protocol)
{
  /* Unset the bit for this protocol in refcount. */
  fib_handle->refcount = fib_handle->refcount & ~(1 << protocol);
}

/*
 * Get the key for an interface.
 */
uint32
mpls_if_key (struct mpls_interface *ifp)
{
  return (uint32)ifp->ifindex;
}

/*
 * Function to compare two interfaces.
 */
int
mpls_if_cmp (struct mpls_interface *ifp1, struct mpls_interface *ifp2)
{
  if (ifp1->ifindex == ifp2->ifindex)
    return 1;
  else
    return -1;
}

/*
 * Routine to GET an interface specific ILM table.
 */
struct mpls_table *
mpls_if_ilm_get (u_int16_t label_space)
{
  struct mpls_table *ilm, *head;

  for (ilm = fib_handle->ilm_list; ilm; ilm = ilm->next)
    {
      if (ilm->ident == (int)label_space)
        {
          /* ILM table found for this label space. */
          ilm->refcount++;
          return ilm;
        }
    }
  
  /* Create a table. */
  ilm = mpls_table_init ((int)label_space);
  if (! ilm)
    return NULL;

  /* Make a copy of the head */
  head = fib_handle->ilm_list;

  /* Set the new VRF to the head. */
  fib_handle->ilm_list = ilm;

  /* Set the next to head, even if head is NULL. */
  ilm->next = head;

  /* If there was already something there, make sure it's
     prev points to the new vrf table. */
  if (head)
    head->prev = ilm;

  /* Update refcount. */
  ilm->refcount++;

  PRINTK_DEBUG ("Created ILM table with ident %d\n", ilm->ident);
  
  /* Return the new ilm table. */
  return ilm;
}

/*
 * Create an interface.
 */
struct mpls_interface *
mpls_if_create (uint32 ifindex, uint16 label_space, uint8 status)
{
  struct mpls_interface *ifp;

  /* First check whether this interface already exists. */
  ifp = mpls_if_lookup (ifindex);
  if (ifp)
    {
      /* It already exists ... */

      /* Just update the interface's label space if required. */
      if (ifp->label_space != label_space)
        {
          /* Disassociate old ILM table from this interface. */
          mpls_if_ilm_unbind (ifp);
          
          /* Update the label space. */
          ifp->label_space = label_space;
          
          ifp->ilm = mpls_if_ilm_get (ifp->label_space);
        }
    }
  else
    {
      ifp = (struct mpls_interface *) kmalloc (sizeof (struct mpls_interface),
                                        GFP_ATOMIC);
      memset (ifp, 0, sizeof (struct mpls_interface));

      /* Set ifindex */
      ifp->ifindex = ifindex;
      
      /* Set label-space */
      ifp->label_space = label_space;

      /* Create an ILM table for this interface, if required */
      ifp->ilm = mpls_if_ilm_get (ifp->label_space);

      /* Set vrf to NULL */
      ifp->vrf = NULL;
      
      /* Add this ifp to the hash table */
      mpls_hash_get (ifp, 1 /* create */);
      
      PRINTK_DEBUG ("Adding Interface with ifindex %d\n",
                    (int)ifp->ifindex);
    }
  
  /* Update status only if it wasnt already MPLS ENABLED */
  if (ifp->status != MPLS_ENABLED)
    ifp->status = status;
  
  /* Update refcount */
  ifp->refcount++;

  return ifp;
}

/*
 * Update an interface's vrf table.
 */
struct mpls_interface *
mpls_if_update_vrf (uint32 ifindex, int ident, int *ret)
{
  struct mpls_interface *ifp;
  struct mpls_table *vrf;

  /* Preset ret to FAILURE */
  *ret = FAILURE;
  
  /* Get the interface */
  ifp = mpls_hash_get_using_key (ifindex);
  if (! ifp)
    {
      if (ident == GLOBAL_TABLE)
        {
          *ret = SUCCESS;
          return NULL;
        }

      /* Create dummy interface. */
      ifp = mpls_if_create (ifindex, 0, MPLS_DISABLED);
      if (! ifp)
        return NULL;
    }
  
  /* Get the vrf table */
  vrf = ifp->vrf;
  
  if (vrf)
    {
      /* Check if this is a redundant operation */
      if (vrf->ident == ident)
        {
          /* Get out */
          return ifp;
        }
      else
        {
          /* Decrement the ref count for the old vrf */
          vrf->refcount--;
        }
    }
  
  /* Set the new one */
  ifp->vrf = mpls_vrf_lookup_by_id (ident, 1 /* create if required */);
  if (ifp->vrf)
    /* Increment refcount. */
    ifp->vrf->refcount++;

  /*
   * If the VRF entry was deleted, and this interface was not 
   * MPLS ENABLED, delete it.
   */
  if ((ifp->vrf == NULL) && (ifp->status != MPLS_ENABLED))
    {
      mpls_if_delete (ifp);
      *ret = SUCCESS;
      return NULL;
    }
  
  *ret = SUCCESS;
  return ifp;
}

/*
 * Delete an interface.
 */
void
mpls_if_delete (struct mpls_interface *ifp)
{
  struct mpls_table *vrf;
  struct mpls_interface *tmp_ifp;
  
  /* Update refcount */
  if (ifp->refcount > 0)
    ifp->refcount--;
  
  if (ifp->refcount > 0)
    return;

  /* Get the vrf that this MPLS interface was pointing to */
  vrf = ifp->vrf;
  if (vrf)
    {
      /* Decrement the refcount for this vrf */
      vrf->refcount--;

      /* Set VRF to NULL */
      ifp->vrf = NULL;
    }

  /* Remove the ILM table for this interface */
  mpls_if_ilm_unbind (ifp);

  /* Remove this interface from the hash table */
  tmp_ifp = mpls_hash_release (ifp);
  if ((struct mpls_interface *) tmp_ifp != ifp)
    PRINTK_WARNING ("Memory corruption occurred\n");

  PRINTK_DEBUG ("Done deleting Interface with ifindex %d\n",
                (int)ifp->ifindex);

  /* If ftn attached, delete it. */
  if (ifp->vc_ftn)
    fib_delete_ftn (ifp->vc_ftn);

  /* Free the interface */
  kfree (ifp);
  
  /* Done */
}

/*
 * Routine to clean up interface data only.
 */
void
mpls_if_delete_only (struct mpls_interface *ifp)
{
  struct mpls_table *vrf;

  /* Get the vrf that this MPLS interface was pointing to */
  vrf = ifp->vrf;
  if (vrf)
    {
      /* Decrement the refcount for this vrf */
      vrf->refcount--;

      /* Set VRF to NULL */
      ifp->vrf = NULL;
    }

  PRINTK_DEBUG ("Done freeing Interface with ifindex %d\n",
                (int)ifp->ifindex);

  /* If some vc id was bound to this interface, unset promiscuity. */
  if (ifp->vc_id != VC_INVALID)
    {
      struct net_device *dev = dev_get_by_index (&init_net, ifp->ifindex);
      dev_set_promiscuity (dev, -1);
    }

  /* If ftn attached, delete it. */
  if (ifp->vc_ftn)
    fib_delete_ftn (ifp->vc_ftn);
  
  /* Free the interface */
  kfree (ifp);
  
  /* Done */
}

/*
 * Create a new VRF with the specified identifier.
 */
struct mpls_table *
mpls_vrf_create (int ident)
{
  struct mpls_table *vrf;
  struct mpls_table *head;

  vrf = mpls_table_init (ident);
  if (vrf == NULL)
    {
      PRINTK_ERR ("Panic !! Could not initialize VRF "
                  "table with identifier %d\n", ident);
      return NULL;
    }

  /*
   * Add this to the top of the list of vrf tables.
   */

  /* Make a copy of the head */
  head = fib_handle->vrf_list;

  /* Set the new VRF to top */
  fib_handle->vrf_list = vrf;

  /* Set previous for new VRF to NULL (Redundant) */
  vrf->prev = NULL;

  /* Set the next to head, even if head is NULL */
  vrf->next = head;

  /* If there was already something there, make sure it's
     prev points to the new vrf table */
  if (head)
    head->prev = vrf;

  PRINTK_DEBUG ("Created vrf table with ident %d\n", ident);
  
  /* Return the new vrf table */
  return vrf;
}

/*
 * Lookup the interface with the passed-in ifindex.
 */
struct mpls_interface *
mpls_if_lookup (uint32 ifindex)
{
  return mpls_hash_get_using_key (ifindex);
}

/*
 * Lookup a VRF table.
 *
 * Create if specified.
 */
struct mpls_table *
mpls_vrf_lookup_by_id (int ident, int create)
{
  struct mpls_table *vrf;

  if (ident == -1 || ident == 0)
    return NULL;

  for (vrf = fib_handle->vrf_list; vrf; vrf = vrf->next)
    {
      if (vrf->ident == ident)
        return vrf;
    }

  /* VRF not found. If create set to 1, create a new one and get out */
  if (create == 1)
    return mpls_vrf_create (ident);
  else
    return NULL;
}

/*
 * Delete a specified VRF.
 */
void
mpls_vrf_delete (int ident)
{
  struct mpls_table *vrf;
  int found = 0;
  
  for (vrf = fib_handle->vrf_list; vrf; vrf = vrf->next)
    {
      if (vrf->ident == ident)
        {
          found = 1;
          break;
        }
    }

  if (found)
    {
      /* First go through all the interfaces, and make sure
         none of 'em point to this vrf. */
      mpls_hash_iterate (mpls_if_unbind_vrf, vrf);

      /* Now rearrange the list */
      if (vrf->prev)
        {
          /* This is not the first one in the list */
          vrf->prev->next = vrf->next;
        }
      else
        {
          /* This is the first one in the list */
          fib_handle->vrf_list = vrf->next;
        }

      if (vrf->next)
        vrf->next->prev = vrf->prev;

      /* Now clean the table entries */
      mpls_destroy_ftn (vrf, APN_PROTO_UNSPEC);

      /* Lastly, delete the vrf table itself */
      mpls_table_finish (vrf);

      PRINTK_DEBUG ("Delete VRF table with ident %d\n", ident);
    }
}

/*
 * Delete all VRF tables.
 *
 * This routine doesnt care about if's pointing to this vrf.
 * This is only a cleanup API.
 */
void
mpls_vrf_delete_all (struct mpls_table *vrf_list)
{
  struct mpls_table *vrf;
  struct mpls_table *vrf_next;

  /* Get out if the vrf list pointer is NULL */
  if (! vrf_list)
    return;

  for (vrf = vrf_list; vrf != NULL; vrf = vrf_next)
    {
      /* Make copy of the next vrf in the list */
      vrf_next = vrf->next;

      /* Move it out of the list */
      fib_handle->vrf_list = vrf->next;

      /* First go through all the interfaces, and make sure
         none of 'em point to this vrf */
      mpls_hash_iterate (mpls_if_unbind_vrf, vrf);
      
      /* Now clean the table entries */
      mpls_destroy_ftn (vrf, APN_PROTO_UNSPEC);
      
      /* Lastly, delete the vrf table itself */
      mpls_table_finish (vrf);
    }

  PRINTK_DEBUG ("Finished deleting all VRF tables\n");
}

/* Clean up all VRF tables for the specified protocol. */
void
mpls_vrf_clean_all (uint8 protocol)
{
  struct mpls_table *vrf;

  /* Get out if the vrf list pointer is NULL. */
  if (! fib_handle->vrf_list)
    return;
  
  for (vrf = fib_handle->vrf_list; vrf != NULL; vrf = vrf->next)
    /* Clean the table entries for protocol match. */
    mpls_destroy_ftn (vrf, protocol);

  PRINTK_DEBUG ("Finished cleaning all VRF tables\n");
}

/*
 * Set the vrf pointer for the specified interface to NULL.
 */
void
mpls_if_unbind_vrf (struct mpls_hash_bucket *bucket, void * arg)
{
  struct mpls_interface *ifp;
  struct mpls_table *ifp_vrf, *vrf;

  /* Get the interface pointer from the hash bucket */
  ifp = bucket->data;
  
  /* Get the ifp vrf */
  ifp_vrf = ifp->vrf;

  /* Get the vrf table */
  vrf = arg;

  if (ifp_vrf && vrf->ident == ifp_vrf->ident)
    {
      /* Set the vrf pointer in the interface to NULL */
      ifp->vrf = NULL;
    }
}

/*
 * Routine to delete the current ILM table in the interface.
 */
void
mpls_if_ilm_unbind (struct mpls_interface *ifp)
{
  struct mpls_table *ilm = ifp->ilm;

  /* Decrement refcount for ILM table. */
  ilm->refcount--;

  if (ilm->refcount <= 0)
    {
      PRINTK_DEBUG ("Deleting ILM table with ident %d\n", ilm->ident);
      
      mpls_destroy_ilm (ilm, APN_PROTO_UNSPEC);

      /* Now rearrange the top ILM list. */
      if (ilm->prev)
        {
          /* This is not the first one in the list. */
          ilm->prev->next = ilm->next;
        }
      else
        {
          /* This is the first one in the list. */
          fib_handle->ilm_list = ilm->next;
        }

      if (ilm->next)
        ilm->next->prev = ilm->prev;

      /* Delete table. */
      mpls_table_finish (ilm);
    }
}

/*
 * Delete all ILMs.
 */
void
mpls_ilm_delete_all (uint8 protocol)
{
  struct mpls_table *ilm, *ilm_next;

  for (ilm = fib_handle->ilm_list; ilm; ilm = ilm_next)
    {
      /* Get next. */
      ilm_next = ilm->next;

      /* Clean up the ILM table and delete it. */
      mpls_destroy_ilm (ilm, protocol);
      mpls_table_finish (ilm);
    }
}

/*
 * Clean all ILMs.
 */
void
mpls_ilm_clean_all (uint8 protocol)
{
  struct mpls_table *ilm, *ilm_next;

  for (ilm = fib_handle->ilm_list; ilm; ilm = ilm_next)
    {
      /* Get next. */
      ilm_next = ilm->next;

      /* Clean up the ILM table for the protocol */
      mpls_destroy_ilm (ilm, protocol);
    }
}

uint32 
get_if_addr (struct net_device *dev, uint8 is_local)
{
  struct in_device *inet_dev = NULL;
  uint32 addr = 0;
  
  inet_dev = in_dev_get (dev);
  if (inet_dev != NULL)
    {
      rcu_read_lock();
      
      for_primary_ifa (inet_dev)
        {
          if (is_local == MPLS_TRUE)
            addr = ifa->ifa_local;
          else
            addr = ifa->ifa_broadcast;
          break;
        }endfor_ifa (inet_dev);
      
      rcu_read_unlock();
      in_dev_put (inet_dev);
    }
  
  return addr;
}

struct net_device *
get_dev_by_addr (uint32 addr)
{
  struct net_device *dev;
  struct in_device *inet_dev = NULL;

  read_lock(&dev_base_lock);
  for_each_netdev(&init_net, dev)
    {
      inet_dev = in_dev_get (dev);
      if (inet_dev == NULL)
        continue;
      rcu_read_lock ();
      for_ifa (inet_dev)
        {
          if (addr == ifa->ifa_local)
            {
        rcu_read_unlock ();
              in_dev_put (inet_dev);
              return dev;
            }
        } endfor_ifa (inet_dev);
      rcu_read_unlock ();
      in_dev_put (inet_dev);
    }
  read_unlock(&dev_base_lock);
 
  return NULL;
}

void
dst_data_init (uint32 addr, struct net_device *dev, uint8 opcode,
               uint8 rt_lookup_type)
{
  struct sk_buff *skb = NULL;
  struct dst_entry *dst = NULL;
  int ret;

  if (dev == NULL)
    {
      PRINTK_ERR ("dst_data_init : Invalid device \n");
      return;
    }

  if (addr == 0)
    {
      if (rt_lookup_type != RT_LOOKUP_OUTPUT)
        return;
      addr = get_if_addr (dev, MPLS_TRUE /* local */);
    }

  skb = alloc_skb (10, GFP_ATOMIC);
  if (skb == NULL)
    {
      PRINTK_ERR ("dst_data_init : skb allocation failed \n");
      return;
    }

  skb->protocol = __constant_htons (ETH_P_IP);
 
  /* ip route lookup */
  if (rt_lookup_type == RT_LOOKUP_INPUT)
    {
      ret = ip_route_input (skb, addr, addr, 0, dev);
      if ((ret != 0) || (skb->dst == NULL))
        {
          PRINTK_DEBUG ("IP route lookup failed for "
                        "destination = %d.%d.%d.%d, dev = %s ret = %d\n", 
                        NIPQUAD (addr), dev->name, ret);

          kfree_skb (skb);
          return;
        }
      dst = skb->dst;
    }
  else
    {
      struct rtable *rt = NULL;
      int ifindex = dev->ifindex;
      struct flowi fli;

      memset (&fli, 0, sizeof (struct flowi));
      fli.fl4_dst = addr;
      fli.fl4_src = addr;
      fli.oif = ifindex;
      fli.iif = ifindex;

      ret = ip_route_output_key (dev_net(dev), &rt, &fli);
      if ((ret != 0) || (rt == NULL))
        {
          PRINTK_DEBUG ("IP route lookup failed for "
                        "destination = %d.%d.%d.%d, dev = %s\n",
                        NIPQUAD (addr), dev->name);

          kfree_skb (skb);
          return;
        }
      dst = &rt->u.dst;
    }


  if (dst->ops == NULL)
    {
      PRINTK_WARNING ("Invalid dst entry \n");
      kfree_skb (skb);
      return;
    }


  if ((dst_ops == NULL) && (opcode & (SET_DST_OPS|SET_DST_ALL)))
    dst_ops = dst->ops;

#if 0
  if ((mpls_dst_input == NULL) && (opcode & (SET_DST_INPUT | SET_DST_ALL)) &&
      (((struct rtable *)dst)->rt_type == RTN_UNICAST))
    mpls_dst_input = dst->input;
#endif 

  kfree_skb (skb);

  if (dst)
  {
    struct dst_entry *dst2 = dst;

    dst_set_expires (dst, 0);
    dst_negative_advice (&dst2);
  }

  return;
}

int
dst_neigh_output (struct sk_buff *skb)
{
  write_lock_bh (&skb->dst->neighbour->lock);
  skb->dst->neighbour->output = neigh_resolve_output;
  write_unlock_bh (&skb->dst->neighbour->lock);
        
  /* To prevent from a crashed machine (some machine happened),
     skb->ip_summed set to CHECKSUM_NONE in dst_neigh_output().
     @ip_summed: Driver fed us an IP checksum.
     Because of mpls_fragment(), this routine locates in here. */
  /* CHECKSUM_PARTIAL for outgoing pkt
   * CHECKSUM_COMPLETE for incomming pkt */
  if (skb->ip_summed == CHECKSUM_PARTIAL) {
    skb->ip_summed = CHECKSUM_NONE;
    PRINTK_DEBUG ("dst_neigh_output : ip_summed of skb set to CHECKSUM_NONE.\n");
  }
  return neigh_resolve_output (skb);
}

void
dst_deinit (struct dst_entry *dst)
{
  struct neighbour *neigh;
  struct hh_cache *hh;
      
  if (! dst)
    return;

  neigh = dst->neighbour;
  hh = dst->hh;
  
  if (hh && atomic_dec_and_test (&hh->hh_refcnt))
    kfree (hh);
      
  dst->hh = NULL;
      
  if (neigh)
    {
      dst->neighbour = NULL;
      write_lock_bh (&neigh->lock);
      neigh->output = neigh_resolve_output;
      write_unlock_bh (&neigh->lock);
      neigh_release (neigh);
    }
      
  if (dst->dev)
    dev_put (dst->dev);
}

int 
mpls_dst_output (struct sk_buff *skb)
{
  struct rtable *rt = (struct rtable *)skb->dst;
  
  kfree_skb (skb);
  if (rt)
    rt_free (rt);
  skb->dst = NULL;
  
  return 0;
}

void 
rt_set (struct rtable *rt, struct dst_ops *ops, uint32 nexthop, 
        struct net_device *dev, int (*input) (struct sk_buff *))
{
  struct dst_entry *dst = NULL;
  
  if ((! rt) || (!dev))
    return;
  
  memset (rt, 0, sizeof (struct dst_entry));
  dst = &rt->u.dst;
  
  atomic_set (&dst->__refcnt, 1);
  dst->__use = 1;
  dst->ops = ops;
  dst->flags = DST_HOST;  
  dst->input = input;
  dst->output = mpls_dst_output;
  dst->dev = dev;
  dev_hold (dev);
  dst->lastuse = jiffies;
  
  dst->metrics[RTAX_MTU-1] = dev->mtu;
  if (dst->metrics[RTAX_MTU-1] > 0xFFF0)
    dst->metrics[RTAX_MTU-1] = 0xFFF0;
  
#if 0
  if (dst->advmss == 0)
    dst->advmss = dev->mtu - 40 > 256 ? dev->mtu - 40 : 256;
#endif
  
  if (dst->metrics[RTAX_ADVMSS-1] == 0)
    dst->metrics[RTAX_ADVMSS-1] = max_t(unsigned int, dst->dev->mtu - 40,
                                        256);
  if (dst->metrics[RTAX_ADVMSS-1] > 65535 - 40)
    dst->metrics[RTAX_ADVMSS-1] = 65535 - 40;

  rt->rt_gateway = nexthop;
  rt->rt_src = 0;
  rt->rt_dst = 0;
  rt->rt_iif = 0;
  rt->rt_spec_dst = 0;
  
  if (nexthop != 0)
    {
      int ret;
      ret = arp_bind_neighbour (dst);
      if (ret != 0)
        {
          PRINTK_ERR ("Error : Neighbour allocation failed \n");
          return;
        }
    }
  rt->rt_flags = 0;
}

/*
 * Routine to create a new rtable entry.
 */
struct rtable *
rt_new (struct dst_ops *ops, uint32 nexthop, struct net_device *dev, 
        int (*input) (struct sk_buff *))
{
  struct rtable *rt;

  rt = kmalloc (sizeof (struct rtable), GFP_ATOMIC);
  if (rt == NULL)
    {
      PRINTK_ERR ("Error : Failed to allocate rtable entry \n");
      return NULL;
    }
  rt_set (rt, ops, nexthop, dev, input);
 
  return rt;
}


void
rt_free (struct rtable *rt)
{
  if (! rt)
    return;

  if (! atomic_read (&rt->u.dst.__refcnt))
    {
      dst_deinit (&rt->u.dst);
      kfree (rt);
    }
}


/*
 * Helper routine to check whether a given interface's
 * label_space is interface_specific (non zero) or not.
 */
int
is_interface_specific (uint32 ifindex)
{
  struct mpls_interface *ifp;

  ifp = mpls_if_lookup (ifindex);
  if (ifp)
    {
      if (ifp->label_space > 0)
        return 1;
    }

  return -1;
}

struct rtable *
rt_make_copy (struct rtable *rt)
{
  struct rtable *rt2 = NULL;
  
  dst_hold (&rt->u.dst);

  rt2 = rt_new (rt->u.dst.ops, rt->rt_gateway, rt->u.dst.dev, rt->u.dst.input);
  if (rt2 == NULL)
    {
      dst_release (&rt->u.dst);
      rt_free (rt);
      return NULL;
    }

  dst_release (&rt->u.dst);
  rt_free (rt);

  return rt2;
}

int
rt_add (struct rtable *rt)
{
  struct rt_node *node;

  node = kmalloc (sizeof (struct rt_node), GFP_ATOMIC);
  if (node == NULL)
    return -1;

  write_lock_bh (&rt_tbl_lock);

  node->rt = rt;
  node->next = rt_head;
  rt_head = node;

  write_unlock_bh (&rt_tbl_lock);

  if (rt_head->next == NULL)
      rt_start_timer ();

  return 0;
}

void
rt_cleanup (unsigned long dummy)
{
  struct rt_node *node = NULL, *prev = NULL;
  int i=0;

  write_lock_bh (&rt_tbl_lock);

  node = rt_head;
  while (node != NULL)
    {
      i++;
      if (atomic_read (&node->rt->u.dst.__refcnt) <= 0)
        {
          i--;
          rt_free (node->rt);
          node->rt = NULL;
          if (prev == NULL)
            {
              rt_head = node->next;
              kfree (node);
              node = rt_head;
            }
          else
            {
              prev->next = node->next;
              kfree (node);
              node = prev->next;
            }
        }
      else
        {
          prev = node;
          node = node->next;
        }
    }

  write_unlock_bh (&rt_tbl_lock);
  
  if (rt_head != NULL)
    mod_timer (&rt_cleanup_timer, (jiffies + 2*60*HZ));
}

void 
rt_start_timer ()
{
  init_timer (&rt_cleanup_timer);
  rt_cleanup_timer.expires = jiffies + 2*60*HZ;
  rt_cleanup_timer.data = 1L;
  rt_cleanup_timer.function = rt_cleanup;
  add_timer (&rt_cleanup_timer);
}


void rt_purge_list (u8 delete_timer)
{
  struct rt_node *node;
  struct rt_node *next = NULL;
  int i=0;
  
  if (delete_timer)
    del_timer_sync (&rt_cleanup_timer);
  
  write_lock_bh (&rt_tbl_lock);
  
  for (node = rt_head; node; node = next)
    {
      next = node->next;
      i++;
      
      if (atomic_read (&node->rt->u.dst.__refcnt) <= 0)
        {
          rt_free (node->rt);
          i--;
        }
      
      node->rt = NULL;
      kfree (node);
    }
  
  rt_head = NULL;
  write_unlock_bh (&rt_tbl_lock);
}

/* Bind an interface to a virtual circuit. */
int
mpls_if_vc_bind (struct net_device *dev, uint32 vc_id)
{
  struct mpls_interface *ifp;

  /* Confirmation. */
  if (! dev)
    return FAILURE;

  /* Get interface. */
  ifp = mpls_if_lookup (dev->ifindex);
  if (! ifp)
    {
      PRINTK_WARNING ("mpls_vc_bind(): Interface with index %d not "
                      "enabled for MPLS\n", (int)dev->ifindex);
      return FAILURE;
    }

  /* Compare VC id. If no change, return. */
  if (ifp->vc_id == vc_id)
    return SUCCESS;

  /* Remove previous label data, if any. */
  if (ifp->vc_ftn)
    {
      fib_delete_ftn (ifp->vc_ftn);
      ifp->vc_ftn = NULL;
    }

  /* Set circuit value. */
  ifp->vc_id = vc_id;

#ifdef HAVE_SWFWDR
  /* Disable L2 switching on this port */ 
  if (dev->apn_fwd_port)
    {
      ifp->l2_info = dev->apn_fwd_port;
      dev->apn_fwd_port = NULL;
    }
#endif /* HAVE_SWFWDR */

  /* Enable promiscuous mode. */
  dev_set_promiscuity (dev, 1);

  return SUCCESS;
}

/* Unind an interface from a virtual circuit. */
int
mpls_if_vc_unbind (struct net_device *dev, uint32 vc_id)
{
  struct mpls_interface *ifp;

  /* Confirmation. */
  if (! dev)
    return FAILURE;

  /* Get interface. */
  ifp = mpls_if_lookup (dev->ifindex);
  if (! ifp)
    {
      PRINTK_WARNING ("mpls_vc_unbind(): Interface with index %d not "
                      "enabled for MPLS\n", (int)dev->ifindex);
      return FAILURE;
    }

  /* Delete the FTN entry. */
  if (ifp->vc_ftn)
    {
      fib_delete_ftn (ifp->vc_ftn);
      ifp->vc_ftn = NULL;
    }
  
  /* Remove virtual circuit id. */
  ifp->vc_id = VC_INVALID;

#ifdef HAVE_SWFWDR 
  /* Enable L2 switching on this port */ 
  if (ifp->l2_info)
    {
      dev->apn_fwd_port = ifp->l2_info;
      ifp->l2_info = NULL;
    }
#endif /* HAVE_SWFWDR */

  /* Disable promiscuous mode. */
  dev_set_promiscuity (dev, -1);

  return SUCCESS;
}

/* Add a VC specific FTN entry to the interface. */
int
mpls_if_add_vc_ftnentry (uint8 protocol, uint32 vc_id, LABEL out_label,
                         L3ADDR nexthop, struct mpls_interface *ifp,
                         struct net_device *dev, uint8 opcode, 
                         uint32 tunnel_ftnix)
{
  struct rtable *rt = NULL;
  uint8 status;

  if (ifp->vc_ftn)
    /* Remove old FTN entry. */
    fib_delete_ftn (ifp->vc_ftn);
  
  /* Create new FTN entry. */
  ifp->vc_ftn = fib_ftn_new (&status);
  if ((ifp->vc_ftn == NULL) || (status != SUCCESS))
    {
      PRINTK_ERR ("Virtual Circuit FTN object for Interface "
                  "with index %d not allocated\n", (int)ifp->ifindex);
      return -ENOMEM;
    }

  ifp->vc_ftn->tunnel_ftnix = tunnel_ftnix; 
 
  /* Allocate rtable entry. */
  rt = rt_new (&labelled_dst_ops, nexthop, dev, mpls_forward);
  if (! rt)
    {
      PRINTK_ERR ("VC FTN entry add error : rt allocation failed\n");
      fib_delete_ftn (ifp->vc_ftn);
      return -ENOMEM;
    }
  
  /* Fill FTN entry. */
  ifp->vc_ftn->protocol = protocol;
  ifp->vc_ftn->nhlfe_list->rt = rt;
  ifp->vc_ftn->nhlfe_list->next = NULL;
  ifp->vc_ftn->nhlfe_list->primitive->next = NULL;
  ifp->vc_ftn->nhlfe_list->primitive->label = out_label;
  ifp->vc_ftn->nhlfe_list->primitive->opcode = opcode;

  PRINTK_DEBUG ("Added Virtual Circuit FTN entry for interface with "
                "index %d and label %d and nexthop %d.%d.%d.%d\n",
                (int)dev->ifindex, (int)out_label, NIPQUAD(nexthop));

  return SUCCESS;
}

/* Delete a VC specific FTN entry from the interface. */
int
mpls_if_del_vc_ftnentry (struct mpls_interface *ifp, uint8 protocol)
{
  if (! ifp->vc_ftn)
    return FAILURE;

  if (ifp->vc_ftn->protocol == protocol)
    {
      /* Delete FTN entry and set it to NULL. */
      fib_delete_ftn (ifp->vc_ftn);
      ifp->vc_ftn = NULL;

      PRINTK_DEBUG ("Removed Virtual Circuit FTN entry for interface with "
                    "index %d\n", (int)ifp->ifindex);

      return SUCCESS;
    }

  return FAILURE;
}

/* Clear out top level stats. */
void
mpls_fib_handle_clear_stats (struct fib_handle *fib_handle)
{
  memset (&fib_handle->stats, 0, sizeof (struct fib_stats));
}

/* Clear out FTN entry stats. */
void
mpls_ftn_entry_clear_stats (struct fib_handle *fib_handle,
                            struct ftn_entry *ftn)
{
  struct mpls_table *vrf;
  struct mpls_table_node *tn;
  struct ftn_entry *ftnp;

  if (ftn)
    {
      memset (&ftn->stats, 0, sizeof (struct ftn_entry_stats));
      return;
    }

  for (vrf = fib_handle->ftn;
       vrf;
       vrf = (vrf == fib_handle->ftn ? fib_handle->vrf_list : vrf->next))
    {
      for (tn = mpls_table_top (vrf); tn; tn = mpls_table_next (tn))
        if ((ftnp = tn->info) != NULL)
          {
            MPLS_ASSERT (ftnp->nhlfe_list->rt != NULL);
            memset (&ftnp->stats, 0, sizeof (struct ftn_entry_stats));
          }
    }
}

/* Clear out ILM entry stats. */
void
mpls_ilm_entry_clear_stats (struct fib_handle *fib_handle,
                            struct ilm_entry *ilm)
{
  struct mpls_table_node *tn;
  struct mpls_table *ilm_table;
  struct ilm_entry *ilmp;

  if (ilm)
    {
      memset (&ilm->stats, 0, sizeof (struct ilm_entry_stats));
      return;
    }

  for (ilm_table = fib_handle->ilm_list;
       ilm_table;
       ilm_table = ilm_table->next)
    for (tn = mpls_table_top (ilm_table); tn; tn = mpls_table_next (tn))
      if ((ilmp = tn->info) != NULL)
        {
          MPLS_ASSERT (ilmp->nhlfe_list->rt != NULL);
          memset (&ilmp->stats, 0, sizeof (struct ilm_entry_stats));
        }
}

void
mpls_send_pkt_over_bypass (int entry_idx, int pkt_len, char *data)
{
  struct sk_buff    *skb = NULL;
  __u8              *eth;
  struct mpls_table *table;
  struct mpls_table_node *tn;
  struct ftn_entry *ftnp = NULL;
  int              ret;

  /* First find the FTN entry based on entry_idx or send the ifIndex and the 
     label entry */
  /* Allocate a buffer of size = pkt size + MPLS header length + 
     Ethernet Header length ) */

  skb = alloc_skb (pkt_len + SHIM_HLEN + ETH_HLEN + 2, GFP_ATOMIC);   
  if (!skb)
    {
      PRINTK_DEBUG ("Memory Allocation issue \n");
      return;
    }

  skb_reserve(skb, 16 + SHIM_HLEN); 
  /* Reserve Ethernet header. Ethernet header length and tail manipulated  */ 
  eth = (__u8 *) skb_push (skb, 14);
  /* Assuming MPLS forward will set the destination length correctly */  

  /* We have the skb head and the data allocated now. We need to fill the buffer
     correct the pointers and send the packet ahead.*/
  /* Copy data */
  skb->data += 14;

  /* skb->data now at the end of ethernet header */
  memcpy (skb->data, data, pkt_len);
  /* Tail pointers and data length manipulated */
  skb_put (skb, pkt_len);
  
  /* Need to fill things here - Set the correct pointers.
     skb->data points to the IP Header
     skb->mac.raw and skb->nh.raw need to be set correctly */
  skb->mac_header = eth;
  skb->network_header = skb->data;
  
  /* Shim Header will be constructed by mpls_pre_forward */
  table = fib_handle->ftn;
  if (!table)
    {
      PRINTK_DEBUG ("FTN table not present \n");
      return;
    }

  for (tn = mpls_table_top (table); tn; tn = mpls_table_next (tn))
    {
      /* Find the entry*/
      if (!tn->info)
          continue;

      ftnp = tn->info
      MPLS_ASSERT (ftnp->nhlfe_list->rt != NULL);
      while (ftnp)
        {
          if (ftnp->ftn_ix == entry_idx)
          break;  
          ftnp = ftnp->next;
        }
      if (ftnp)
        break;

    }

  if (!ftnp)
    {
      PRINTK_DEBUG ("FTN table entry not present \n");
      return;      
    }
 
  ret = mpls_nhlfe_process_opcode (&skb, ftnp->nhlfe_list,
                                       MPLS_FALSE, MPLS_FALSE, 0, 0);
  switch (ret)
    {
      case MPLS_LABEL_FWD:
          /* Release old dst. */
          if ((skb)->dst)
            {
              dst_release ((skb)->dst);
              (skb)->dst = NULL;
            }

          /* Forward labelled packet. */
          mpls_pre_forward ((skb), ftnp->nhlfe_list->rt,
                            MPLS_LABEL_FWD);

          ret = 0;
          break;

      case FTN_LOOKUP:
      case LOCAL_IP_FWD:
      default :
          ret = -1;
    }

  return;
}

void
mpls_send_pkt_for_oam (int ftn_id, int pkt_len, char *data, 
                       int vrf_id, int flags, int ttl)
{
  struct sk_buff    *skb = NULL;
  struct mpls_table *table;
  struct ftn_entry *ftnp = NULL;
  int              ret;

  skb = (struct sk_buff *) mpls_form_skbuff (pkt_len, data);
  if (!skb)
    {
      PRINTK_DEBUG ("Alloc failed for SKBUff \n");
      return;
    }
  
  /* First find the FTN entry based on entry_idx or send the ifIndex and the 
     label entry */
  table = mpls_oam_get_table (vrf_id);
   if (!table)
    {
      PRINTK_DEBUG ("MPLS Table not found \n");
      kfree_skb (skb);
      return;
    }
  
  ftnp = mpls_oam_get_ftn_entry (ftn_id, table);
  if (!ftnp)
    {
      PRINTK_DEBUG ("FTN table entry not present \n");
      kfree_skb (skb);
      return;      
    }

  ret = mpls_oam_do_lookup_fwd (skb, ftnp, ttl, flags);
  if (ret == DO_SECOND_FTN_LOOKUP)
    ret = mpls_oam_do_second_lookup_fwd (ftnp, INVALID_FTN_IX, skb, ttl, 0);
 
  if (ret < 0)
      kfree_skb (skb);
  
  return;
}

struct sk_buff *
mpls_form_skbuff(int pkt_len, char *data)
{
  struct sk_buff *skb = NULL;
  __u8              *eth;

  /* Allocate a buffer of size = pkt size + MPLS header length +  PW ACH Len
       Ethernet Header length ) */
  skb = alloc_skb (pkt_len + 3 * SHIM_HLEN + PW_ACH_LEN + ETH_HLEN + 2 , GFP_ATOMIC);   
    if (!skb)
      {
        PRINTK_DEBUG ("Memory Allocation issue \n");
        return NULL;
      }
  
  skb_reserve(skb, 16 + 3 * SHIM_HLEN + PW_ACH_LEN); 
  /* Reserve Ethernet header. Ethernet header length and tail manipulated  */ 
  eth = (__u8 *) skb_push (skb, 14);
  /* Assuming MPLS forward will set the destination length correctly */  

  /* We have the skb head and the data allocated now. We need to fill the buffer
     correct the pointers and send the packet ahead.*/
  /* Copy data */
  skb->data += 14;

  /* skb->data now at the end of ethernet header */
  memcpy (skb->data, data, pkt_len);
  /* Tail pointers and data length manipulated */
  skb_put (skb, pkt_len);
  
  /* Need to fill things here - Set the correct pointers.
     skb->data points to the IP Header
     skb->mac.raw and skb->nh.raw need to be set correctly */
  skb->mac_header = eth;
  skb->network_header = skb->data;
  return skb;
}

/**@brief - Send the OAM VC Packet 
 * @param vc_id - VC identifier
 * @param pkt_len - Length of the packet
 * @param data - Actual packet 
 * @param ifindex - VC access interface index 
 * @param cc_type - If VCCV packet, contains the CC type, otherwise 0
 * @param cc_input - If cc_type is non zero, additional input
 * @param flags - flags received from the user
 * @param ttl - TTL to be set in the Tunnel label 
 */
void
mpls_send_pkt_for_oam_vc (int vc_id, int pkt_len, char *data, int ifindex, 
                          int cc_type, int cc_input,
                          int flags, int ttl)
{
  struct sk_buff    *skb = NULL;
  struct rtable     *rt;
  struct ftn_entry  *ftnp = NULL;
  int               ret;
  int               vc_ttl = MAX_TTL;
  uint32_t          ralert = 0;
  struct shimhdr* shimhp;

  skb = (struct sk_buff *) mpls_form_skbuff (pkt_len, data);
  if (!skb)
    {
      PRINTK_DEBUG ("Alloc failed for SKBUff \n");
      return;
    }
 
  /* If CC type is 1, Push the PW ACH Header with channel type set to the
   * value received from the user in cc_input */
  if (cc_type == CC_TYPE_1)
    PUSH_PW_ACH (skb, cc_input)
 
  /* If CC type is 3, set the TTL in VC Label to 1 */
  else if (cc_type == CC_TYPE_3) 
    vc_ttl = 1;

  /* First find the FTN entry based on entry_idx or send the ifIndex and the 
     label entry */
 
  ftnp = mpls_oam_get_vc_ftn_entry (vc_id, ifindex);
  if (!ftnp)
    {
      PRINTK_DEBUG ("FTN table entry not present \n");
      kfree_skb (skb);
      return;      
    }

  ret = mpls_oam_do_lookup_fwd (skb, ftnp, vc_ttl, 0);

  /* If CC type is set to 2, push the ROUTER ALERT LABEL before pushing
   * the VC Label below */
  if (cc_type == CC_TYPE_2)
    PUSHLABEL (skb, ROUTERALERT, 0);

  if (ret == DO_SECOND_FTN_LOOKUP)
    ret = mpls_oam_do_second_lookup_fwd (ftnp, INVALID_FTN_IX, skb, ttl, 1);

  if (ret < 0)
    {
      kfree_skb (skb);
      PRINTK_DEBUG ("mpls_send_pkt_for_oam_vc: Failed to send OAM " 
                    "packet for vc_id = %lu, packet len = %lu", vc_id,
                    pkt_len);
      return;
    }
  else 
    PRINTK_DEBUG ("mpls_send_pkt_for_oam_vc: Sent VC OAM packet for vc_id: %lu "
                  "with packet length: %lu", vc_id, pkt_len);
 
  return;
}

struct mpls_table *
mpls_oam_get_table (int vrf_id)
{
  if (vrf_id)
          return (mpls_vrf_lookup_by_id (vrf_id, 0));

  return fib_handle->ftn;
}

struct ftn_entry *
mpls_oam_get_ftn_entry (int ftn_id, struct mpls_table *table)
{
  struct mpls_table_node *tn;
  struct ftn_entry *ftnp = NULL;

  for (tn = mpls_table_top (table); tn; tn = mpls_table_next (tn))
    {
      /* Find the entry*/
      if (!tn->info)
          continue;

      ftnp = tn->info
      MPLS_ASSERT (ftnp->nhlfe_list->rt != NULL);
      while (ftnp)
        {
          if (ftnp->ftn_ix == ftn_id)
            break;  

          ftnp = ftnp->next;
        }

      if (ftnp)
        break;
    }
  return ftnp;
}

int
mpls_oam_do_lookup_fwd (struct sk_buff *sk, struct ftn_entry *ftnp, 
                        int ttl, int flags)
{
  int ret;
  
  ret = mpls_nhlfe_process_opcode (&sk, ftnp->nhlfe_list, 
				   MPLS_FALSE, MPLS_FALSE,
                                   ttl, flags);

  if (ret == MPLS_LABEL_FWD)
  {
    if ((sk)->dst)
      {
         dst_release ((sk)->dst);
         (sk)->dst = NULL;
      }

     /* Forward labelled packet. */
     mpls_pre_forward ((sk), ftnp->nhlfe_list->rt,
                        MPLS_LABEL_FWD);
     return 0;
  }

 return ret;
}

int
mpls_oam_do_second_lookup_fwd (struct ftn_entry *ftnp, int ftn_id, 
                               struct sk_buff *sk, int ttl_val, int8_t for_vpn)
{
  struct ftn_entry *ftn;
  uint32 d_addr;
  int ret;

  d_addr = ftnp->nhlfe_list->rt->rt_gateway;
  if (d_addr ==0)
    return (FAILURE);
  
  ftn = fib_best_ftnentry (fib_handle->ftn, sk, d_addr, RT_LOOKUP_OUTPUT, 
                           for_vpn);
  if (!ftn)
    {
      PRINTK_DEBUG ("Best FTN Entry not present \n");
      return (FAILURE);
    }
   
   ret = mpls_nhlfe_process_opcode (&sk, ftn->nhlfe_list,
				    MPLS_FALSE, MPLS_FALSE,
                                    ttl_val, 0);
   if (ret == MPLS_LABEL_FWD)
     {
       /* Release old dst. */
       if ((sk)->dst)
         {
           dst_release ((sk)->dst);
           (sk)->dst = NULL;
         }

       /* Forward labelled packet. */
       mpls_pre_forward ((sk), ftn->nhlfe_list->rt,
                         MPLS_LABEL_FWD);
       return 0;
     }

 return ret;  
}

struct ftn_entry *
mpls_oam_get_vc_ftn_entry (uint32 vc_id, uint32 ifindex)
{
  struct ftn_entry *ftnp = NULL;
  struct mpls_interface *ifp;
  
  ifp = mpls_if_lookup (ifindex);
   
  if (ifp == NULL)
    {
      PRINTK_DEBUG ("IF table not present \n");
      return NULL;
    }

  if (((ftnp = ifp->vc_ftn) == NULL) ||
       (ifp->vc_id != vc_id))
    {
      PRINTK_DEBUG ("VC table not present \n");
      return NULL;
    }

 return ftnp;
}
