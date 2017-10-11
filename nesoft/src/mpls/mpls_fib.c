/* Copyright (C) 2001-2003  All Rights Reserved.  */

#include "mpls_fwd.h"

#include "mpls_common.h"
#include "mpls_client.h"

#include "mpls_fib.h"
#include "forwarder.h"
#include "mpls_hash.h"
#include "mpls_table.h"
#include "fncdef.h"

extern struct dst_ops *dst_ops;
#if 0
extern int (*mpls_dst_input) (struct sk_buff *);
#endif
extern struct dst_ops labelled_dst_ops;

struct net_device *dev_get_by_index(struct net *net, int ifindex);
struct net_device *dev_get_by_name(struct net *net, const char *name);

/*===========================================================================
 *                                APIs
 *==========================================================================*/

/*
 * fib exports the handle for use by all the other modules
 */
struct fib_handle *fib_handle = NULL;


/*
 * Find the best match entry in the FTN table for the incoming packet.
 */
struct ftn_entry *
fib_best_ftnentry (struct mpls_table *table, struct sk_buff *skb,
                   uint32 daddr, uint8 type, uint8 for_vpn)
{
  struct net_device *dev;
  struct ftn_entry *entry;
  struct mpls_table_node *tn;
  struct flowi fli;
  struct fib_result res;
  struct net *net;
  int ret;
  
  /* Make sure ftn table exists */
  if (! table)
    return NULL;

  /* Find the best match node in the FTN */
  tn = mpls_table_node_match_ipv4 (table, daddr);
  if (tn)
    {
      /* Unlock the node */
      mpls_table_unlock_node (tn);
      
      /*
       * Info will always be there.
       */
      
      entry = tn->info;
    }
  else
    return NULL;

  /* If this a VPN lookup, just return FTN entry. */
  if (for_vpn)
    return entry;

  /* Fill key. */
  memset (&fli, 0, sizeof (struct flowi));
  fli.fl4_dst = daddr;
  fli.fl4_src = ip_hdr(skb)->saddr;
  fli.oif = 0;
  fli.fl4_scope = RT_SCOPE_UNIVERSE;


  if (type == RT_LOOKUP_OUTPUT) /* Locally generated packet. */
    {
      dev = get_dev_by_addr (ip_hdr(skb)->saddr);
      if (! dev)
        return NULL;

      fli.iif = dev->ifindex;
    }
  else /* Incoming packet. */
    {
      fli.fl4_tos = ip_hdr(skb)->tos;
      fli.iif = skb->dev->ifindex;
      dev = skb->dev;
    }
      
  /* Lookup in IPv4 table and get best match amongst IP and MPLS . */
  net = dev_net(dev);
  ret = mpls_ip_fib_lookup(net, &fli, &res);
  if (ret == 0 && entry->fec.prefixlen < res.prefixlen)
    return NULL;

  return entry;
}

/* Allocate memory for a new FTN. */
struct ftn_entry *
fib_ftn_new (uint8 *status)
{
  struct ftn_entry *new_ftn;

  /* Create the ftn entry */
  new_ftn = (struct ftn_entry *)kmalloc(sizeof(struct ftn_entry), GFP_ATOMIC);
  if(new_ftn == NULL)
    {
      *status = -ENOMEM;
      return NULL;
    }
  memset (new_ftn, 0, sizeof (struct ftn_entry));

  /* Initialize ftn tunnel_ftnix to 0 */
  new_ftn->tunnel_ftnix = 0;

  /* Create an empty NHLFE for this FTN */
  new_ftn->nhlfe_list =
    (struct nhlfe_entry *)kmalloc(sizeof(struct nhlfe_entry), GFP_ATOMIC);
  if(new_ftn->nhlfe_list == NULL)
    {
      kfree(new_ftn);
      *status = -ENOMEM;
      return NULL;
    }
  memset (new_ftn->nhlfe_list, 0, sizeof (struct nhlfe_entry));

  /* Create an empty label in the nhlfe */
  new_ftn->nhlfe_list->primitive = 
    (struct label_primitive *)kmalloc(sizeof(struct label_primitive),
                                      GFP_ATOMIC);
  if(new_ftn->nhlfe_list->primitive == NULL)
    {
      kfree(new_ftn->nhlfe_list);
      kfree(new_ftn);
      *status = -ENOMEM;
      return NULL;
    }
  memset (new_ftn->nhlfe_list->primitive, 0, sizeof (struct label_primitive));

  *status = SUCCESS;
  return new_ftn;
}

/*---------------------------------------------------------------------------
  Function to add an entry to the FTN table.
  'fec' is the FEC that is served by this NHLFE entry.
  'nexthop' is the address of the next hop where the unlabelled packet after 
  labelling should be sent.
  'outl2id' is the interface id of the interface on which it should be sent.
  'label' is the label that should be pushed on the message.
  It returns a pointer to the newly added FTN entry.
  --------------------------------------------------------------------------*/
sint8
fib_add_ftnentry(struct mpls_table *table, uint8 protocol,
                 struct mpls_prefix * fec,
                 L3ADDR nexthop, uint32 outl2id,
                 LABEL label, sint8 opcode, uint32 nhlfe_ix, uint32 ftn_ix,
                 struct mpls_owner_fwd *owner, 
		 uint32 bypadd_ftn_ix, u_char lsp_type,
		 int active_head)
{
  struct ftn_entry         *new_ftn = NULL;
  uint8             status;
  struct net_device *out_dev;
  struct dst_ops *ops = NULL;
  int (*input_fn) (struct sk_buff *) = NULL;
  struct rtable *rt = NULL;
  int ret;

  MPLS_ASSERT(fib_handle && fec && label != IMPLICITNULL &&
         ((opcode == PUSH) || (opcode == PUSH_AND_LOOKUP) ||
          (opcode == DLVR_TO_IP) || (opcode == FTN_LOOKUP) ||
          (opcode == PUSH_FOR_VC) || (opcode == PUSH_AND_LOOKUP_FOR_VC)));
  
  /* If case of Virtual Circuits, no table should have been passed. */
  if ((opcode != PUSH_FOR_VC) && (opcode != PUSH_AND_LOOKUP_FOR_VC))
    MPLS_ASSERT (table);
  
  /* In case of DLVR_TO_IP, nexthop can be zero, so it's not required */
  if (opcode != DLVR_TO_IP)
    MPLS_ASSERT (nexthop);
  
  if ((out_dev = dev_get_by_index(&init_net, outl2id)) == NULL)
    {
      PRINTK_ERR ("fib_add_ftnentry : No device with index %d\n",
                  (int)outl2id);
      return -ENODEV;
    }

  /* Get dst attributes. */
  if (opcode == DLVR_TO_IP)
    {
      ops = dst_ops;
      input_fn = mpls_forward;
    }
  else
    {
      ops = &labelled_dst_ops;
      input_fn = mpls_forward;
    }

  /* Allocate rtable entry. */
  rt = rt_new (ops, nexthop, out_dev, input_fn);
  if (rt == NULL)
    {
      PRINTK_ERR ("FTN entry add error : rtable allocation failed\n");
      return -ENOMEM;
    }
  
  /* Lookup this prefix in the FTN table */
  new_ftn = fib_find_ftnentry (table, protocol, fec, ftn_ix, FIB_FIND, &ret);
  if ((new_ftn == NULL) || (ret != SUCCESS))
    {
      new_ftn = allocate_ftn (table, fec, &status, lsp_type, active_head);
      if ((status != SUCCESS) || (new_ftn == NULL))
        {
          PRINTK_ERR ("fib_add_ftnentry : Panic !! No "
                      "resources to allocate for a new FTN entry\n");

          dst_release (&rt->u.dst);
          rt_free (rt);
          return -ENOMEM;
        }
    }

  if (! new_ftn)
    {
      PRINTK_ERR ("fib_add_ftnentry : Panic !! No "
                  "resources to allocate for a new FTN entry\n");

      dst_release (&rt->u.dst);
      rt_free (rt);
      return -ENOMEM;
    }
  else
    {
      /*
       * FEC for which we are creating this FTN entry
       */
      if (new_ftn->fec.u.prefix4.s_addr == fec->u.prefix4.s_addr &&
          new_ftn->fec.prefixlen == fec->prefixlen &&
          new_ftn->nhlfe_list->primitive->label == label &&
          new_ftn->nhlfe_list->rt->rt_gateway == nexthop &&
          new_ftn->nhlfe_list->rt->u.dst.dev == out_dev)
        {
          PRINTK_DEBUG ("fib_add_ftnentry : Exact entry exists\n");
          /*
           * exactly same entry already present
           */

          dst_release (&rt->u.dst);
          rt_free (rt);
          return -EEXIST;
        }
      
      dst_release (&new_ftn->nhlfe_list->rt->u.dst);
      rt_free (new_ftn->nhlfe_list->rt);
      new_ftn->nhlfe_list->rt = NULL;
    }

  new_ftn->nhlfe_list->rt = rt;
  
  /* Copy over the fec */
  new_ftn->fec = *fec;
  
  /* Set the protocol */
  new_ftn->protocol = protocol;
  
  /* Set the owner information */
  if (owner->protocol == APN_PROTO_RSVP)
    memcpy (&new_ftn->owner, owner, sizeof (struct mpls_owner_fwd));

  new_ftn->nhlfe_list->next = NULL;
  
  new_ftn->nhlfe_list->primitive->next = NULL;
  new_ftn->nhlfe_list->primitive->label = label;
  new_ftn->nhlfe_list->primitive->opcode = opcode;

  /* Set the NHLFE index */
  new_ftn->nhlfe_list->nhlfe_ix = nhlfe_ix;
  new_ftn->ftn_ix = ftn_ix;

  new_ftn->lsp_type = lsp_type;

  PRINTK_DEBUG ("Added FTN entry for [%d.%d.%d.%d/%d] with "
                "using label %d and nexthop %d.%d.%d.%d of ftn index %d \n",
                NIPQUAD(fec->u.prefix4.s_addr), fec->prefixlen, (int)label,
                NIPQUAD(nexthop), ftn_ix);
  
  /*
   * FTN entry installed successfully
   */
  return (SUCCESS);
}

/* Delete an FTN entry. */
void
fib_delete_ftn (struct ftn_entry *ftn)
{
  if (ftn)
    {
      if (ftn->nhlfe_list)
        {
          /* Clean label primitive */
          if (ftn->nhlfe_list->primitive)
            kfree (ftn->nhlfe_list->primitive);
          
          /* Clean rtable entry */
          if (ftn->nhlfe_list->rt)
            {
              dst_release (&ftn->nhlfe_list->rt->u.dst);
              rt_free (ftn->nhlfe_list->rt);
              ftn->nhlfe_list->rt = NULL;
            }
          kfree (ftn->nhlfe_list);
        }
      kfree (ftn);
    }
}

 /* Delete the FTN entry from the ftn linklist corresponding to the FEC */
int
delete_ftn_from_list( struct mpls_table_node *tn, struct ftn_entry *ftn)
{
  struct ftn_entry *temp;

  if ( tn->info == ftn)
    {
       /* update the linklist appropriately */
      if ( ftn->next)
        {
          tn->info = ftn->next;
        }
      else
        {
          /* if this is the only FTN  for the FEC then delete the table node */
          tn->info = NULL;
          mpls_table_unlock_node (tn);
        }
    }

  else
    {
      temp = tn->info;

     /* update the linklist appropriately */
      while (temp->next != ftn)
        {
          temp = temp->next;
          if (temp == NULL)
            return FAILURE;
        }
      temp->next = ftn->next;
    }

  return SUCCESS;

}

/*---------------------------------------------------------------------------
  Function to find the FTN entry corresponding to the FEC fec. It returns 
  a pointer to the found FTN entry if any, NULL otherwise. FTN table is
  ALWAYS searched by the exact match.
  --------------------------------------------------------------------------*/
struct ftn_entry *
fib_find_ftnentry(struct mpls_table *table, uint8 protocol,
                  struct mpls_prefix * fec, uint32 ftn_ix, int flag, int *ret)
{
  int retval;
  struct ftn_entry         *ftn;
  struct mpls_table_node *tn;

  *ret = FAILURE;
  retval = FAILURE;

  MPLS_ASSERT(table && fec);

  tn = mpls_table_node_lookup (table, fec);
  if (tn)
    {
      mpls_table_unlock_node (tn);

      /* Make a copy of the info in the route node */
      ftn = tn->info;

      while (ftn)
        {
          if (ftn->ftn_ix == ftn_ix)
            break;

	  ftn = ftn->next;
	}

      if (ftn && flag == FIB_DELETE)
        {
          /* Make sure that the protocol matches */
          if (ftn->protocol != protocol)
            return NULL;

          PRINTK_DEBUG ("Deleting FTN entry for FEC [%d.%d.%d.%d/%d]\n",
                        NIPQUAD (fec->u.prefix4.s_addr), fec->prefixlen);

          /* Delete the FTN entry. */
          retval = delete_ftn_from_list (tn, ftn);

	  if (retval == SUCCESS)
            {
              /*Delete the FTN entry. */
              fib_delete_ftn (ftn);

	      *ret = SUCCESS;
	    }
	  return NULL;
        }
    }
  else
    ftn = NULL;

  if (ftn)
    *ret = SUCCESS;
  
  return ftn;
}

/*---------------------------------------------------------------------------
 * Function to install an ILM entry. If out_label is INVALID, it installs an 
 * incomplete entry and sets the 'status' flag to indicate that. If we already
 * have a complete ILM entry it does nothing.
 --------------------------------------------------------------------------*/
sint8
fib_add_ilmentry(uint8 protocol,
                 LABEL in_label, uint32 inl2id,
                 struct mpls_prefix *fec,
                 L3ADDR nexthop, uint32 outl2id,
                 LABEL out_label, sint8 opcode,
                 uint32 nhlfe_ix, struct mpls_owner_fwd *owner)
{
  struct ilm_entry *new_ilm;
  uint8 status;
  struct net_device *out_dev;
  struct dst_ops *ops = NULL;
  int (*input_fn) (struct sk_buff *) = NULL;
  struct rtable *rt = NULL;
  int ret;
  struct mpls_interface *ifp;
  
  if ((out_dev = dev_get_by_index(&init_net, outl2id)) == NULL)
    {
      PRINTK_ERR ("fib_add_ilmentry : No device with index %d\n",
                  (int)outl2id);
      return -ENODEV;
    }

  /* Get the interface */
  ifp = mpls_if_lookup (inl2id);
  if (! ifp)
    {
      PRINTK_WARNING ("Failed to lookup interface %d during "
                      "add_ilm_entry\n", (int)inl2id);
      return -EINVAL;
    }

  /*
   * in_label should be a valid label, because otherwise we cannot find
   * the index into the hash.If only inlabel is valid then its an 
   * incomplete entry and cannot be used as is, till we get the corresponding 
   * outlabel. An incomplete entry is marked invalid for use.
   */
  if (!(fib_handle && (in_label != INVALID)) ||
      ((opcode != POP_FOR_VPN) && (opcode != POP) && (opcode != SWAP) &&
       (opcode != SWAP_AND_LOOKUP) && (opcode != POP_FOR_VC)))
    {
      PRINTK_ERR ("add_ilm_entry : Invalid arguments \n");
      return -EINVAL;
    }

  /* get dst attributes */
  ops = NULL;
  input_fn = NULL;

  if ((out_label == IMPLICITNULL) &&
      (opcode == SWAP))
    opcode = POP;

  if (opcode == SWAP)
    {
      ops = &labelled_dst_ops;
      input_fn = mpls_forward;
    }

  if (opcode == POP && LOOPBACK (htonl(nexthop)))
     nexthop = 0;

  /* allocate rtable entry */
  rt = rt_new (ops, nexthop, out_dev, input_fn);
  if (rt == NULL)
    {
      PRINTK_ERR ("ILM entry add error : rtable allocation failed\n");
      return -ENOMEM;
    }
  
  /* Lookup this ilm entry in the ILM table first */
  new_ilm = fib_find_ilmentry (ifp->ilm, protocol, in_label, inl2id,
                               FIB_FIND, &ret);
  if (new_ilm && (ret == SUCCESS))
    {
      if (new_ilm->label == in_label &&
          new_ilm->nhlfe_list->primitive->label == out_label &&
          new_ilm->nhlfe_list->primitive->opcode == opcode &&
          new_ilm->inl2id == inl2id &&
          new_ilm->nhlfe_list->rt->rt_gateway == nexthop && 
          new_ilm->nhlfe_list->rt->u.dst.dev == out_dev)
        {
          dst_release (&rt->u.dst);
          rt_free (rt);
          return -EEXIST;
        }
      
      dst_release (&new_ilm->nhlfe_list->rt->u.dst);
      rt_free (new_ilm->nhlfe_list->rt);
      new_ilm->nhlfe_list->rt = NULL;
    }
  else
    {
      struct mpls_prefix p;

      /* Create the prefix for the in label */
      memset (&p, 0, sizeof (struct mpls_prefix));
      p.u.prefix4.s_addr = net32_prefix (in_label, 20);
      p.family = AF_INET;
      p.prefixlen = 20;

      new_ilm = allocate_ilm (ifp->ilm, &p, &status);
      if ((status != SUCCESS) || (new_ilm == NULL))
        {
          PRINTK_ERR ("fib_add_ilmentry : Panic !! No "
                      "resources to allocate for a new ILM entry\n");
          dst_release (&rt->u.dst);
          rt_free (rt);
          return -ENOMEM;
        }
    }
  
  /*
   * set the various fields here
   */

  new_ilm->nhlfe_list->rt = rt;    
  new_ilm->label = in_label;
  new_ilm->status |= VALID;

  /* Set the incoming label space */
  new_ilm->inl2id = inl2id;

  /* Set the fec */
  new_ilm->fec = *fec;

  /* Set the protocol */
  new_ilm->protocol = protocol;

  /* Set the owner information */
  if (owner->protocol == APN_PROTO_RSVP)
    memcpy (&new_ilm->owner, owner, sizeof (struct mpls_owner_fwd));

  new_ilm->nhlfe_list->next = NULL;

  /*
   * outgoing label and the associated opcode
   */
  new_ilm->nhlfe_list->primitive->next = NULL;
  new_ilm->nhlfe_list->primitive->label = out_label;
  new_ilm->nhlfe_list->primitive->opcode = opcode;

  /* Set the NHLFE index */
  new_ilm->nhlfe_list->nhlfe_ix = nhlfe_ix;

  if (opcode == POP_FOR_VC)
    PRINTK_DEBUG ("Added Virtual Circuit ILM entry with incoming "
                  "label %d and nexthop %d.%d.%d.%d. Incoming interface "
                  "is %d and outgoing interface is %d\n",
                  (int)in_label, NIPQUAD(nexthop), (int)inl2id, (int)outl2id);
  else
    PRINTK_DEBUG ("Added ILM entry %d --> %d for "
                  "fec [%d.%d.%d.%d/%d] and nexthop %d.%d.%d.%d "
                  "to ILM table for interface %d\n",
                  (int)in_label, (int)out_label,
                  NIPQUAD(new_ilm->fec.u.prefix4.s_addr),
                  new_ilm->fec.prefixlen,
                  NIPQUAD(nexthop), (int)ifp->ifindex);
  
  /*
   * ILM entry installed successfully
   */

  return SUCCESS;
}

/*---------------------------------------------------------------------------
  Does an ILM table lookup. 'in_label' and 'l2ifid' uniquely identify a label,
  this is so because l2ifid represents a labelspace and in_label identifies
  a label in that labelspace. For now we have only one label space for all
  the interfaces so l2ifid is not effective, returns NULL if no matching
  entry found.
  -------------------------------------------------------------------------*/

struct ilm_entry *
fib_find_ilmentry (struct mpls_table *table, uint8 protocol, LABEL in_label,
                   uint32 inl2id, int flag, int *ret)
{
  struct ilm_entry *ilm;
  struct mpls_table_node *tn;
  struct mpls_prefix p;

  *ret = FAILURE;
  
  MPLS_ASSERT(fib_handle && (in_label != INVALID));

  /* Create the prefix for the in label */
  memset (&p, 0, sizeof (struct mpls_prefix));
  p.u.prefix4.s_addr = net32_prefix (in_label, 20);
  p.family = AF_INET;
  p.prefixlen = 20;

  tn = mpls_table_node_lookup (table, (struct mpls_prefix *)&p);
  if (tn)
    {
      mpls_table_unlock_node (tn);

      /* Make a copy of the info in the route node */
      ilm = tn->info;

      /* This will never happen */
      if (ilm == NULL)
        return NULL;

      if (flag == FIB_DELETE)
        {
          /* Make sure the protocol matches */
          if (ilm->protocol != protocol)
            return NULL;
          
          PRINTK_DEBUG ("Deleting ILM entry for label %d\n",
                        (int)in_label);
          
          if (ilm->nhlfe_list)
            {
              /* Clean primitive */
              if (ilm->nhlfe_list->primitive)
                kfree (ilm->nhlfe_list->primitive);
              
              /* Clean DST entry */
              if (ilm->nhlfe_list->rt)
                {
                  dst_release (&ilm->nhlfe_list->rt->u.dst);
                  rt_free (ilm->nhlfe_list->rt);
                  ilm->nhlfe_list->rt = NULL;
                }
              kfree (ilm->nhlfe_list);
            }

          /* Free the ILM entry */
          kfree (ilm);
          
          /* Set the tn info to NULL */
          tn->info = NULL;

          /* Delete the table node */
          mpls_table_unlock_node (tn);

          *ret = SUCCESS;

          return NULL;
        }
    }
  else
    return NULL;

  /* Check the status as well */
  if (ilm->status & VALID)
    {
      *ret = SUCCESS;
      return ilm;
    }
  else
    return NULL;
}
  
/* Function to add FTN to the list of FTNs with same FEC */
void 
add_ftn_to_list ( struct mpls_table_node *node, struct ftn_entry *ftn, 
                  u_char lsp_type, int active_head)
{
  struct ftn_entry *head;
  struct ftn_entry *temp;

 /* If there is already a ftn entry corresponding to this FEC 
    then add the new entry to the list */ 
  if (node->info)
    { 
      head = node->info;

      /* If new entry is nsm selected active head, dependent entry will
         add to it, so add to head of the link list */
      if (active_head)
        {
          node->info = ftn;
          ftn->next = head;
	}
      /* Add to tail */
      else
        {
          temp = node->info;
          while (temp->next != NULL)
            {
              temp = temp->next;
            }
          temp->next = ftn;
          ftn->next = NULL;
	}
    }
  else
    {
      node->info = ftn;
      ftn->next = NULL;
    }

}  

/*
 * The following routine creates a new FTN entry.
 */
struct ftn_entry *
allocate_ftn (struct mpls_table *table, struct mpls_prefix *fec,
              uint8 *status, u_char lsp_type, int active_head)
{
  struct mpls_table_node *tn;
  struct ftn_entry *new_ftn;

  /* Get a node for this fec */
  tn = mpls_table_node_get (table, fec);
  if (tn == NULL)
    {
      *status = -ENOMEM;
      return NULL;
    }

  /* New FTN. */
  new_ftn = fib_ftn_new (status);
  if (! new_ftn)
    return NULL;

  add_ftn_to_list (tn, new_ftn, lsp_type, active_head);

  /* Set status to SUCCESS */
  *status = SUCCESS;
  
  /* Return new ftn */
  return new_ftn;
}

/*
 * The following routine allocates a new ILM entry.
 */
struct ilm_entry *
allocate_ilm (struct mpls_table *table, struct mpls_prefix *p,
              uint8 *status)
{
  struct mpls_table_node *tn;
  struct ilm_entry *new_ilm;

  /* Get a node for this prefix */
  tn = mpls_table_node_get (table, p);
  if (tn == NULL)
    {
      *status = -ENOMEM;
      return NULL;
    }

  /* Create the ILM entry */
  new_ilm = (struct ilm_entry *)kmalloc(sizeof(struct ilm_entry), GFP_ATOMIC);
  if(new_ilm == NULL)
    {
      *status = FAILURE;
      return NULL;
    }
  memset (new_ilm, 0, sizeof (struct ilm_entry));

  /* Create an empty NHLFE for this ILM */
  new_ilm->nhlfe_list = 
    (struct nhlfe_entry *)kmalloc(sizeof(struct nhlfe_entry), GFP_ATOMIC);
  if(new_ilm->nhlfe_list == NULL)
    {
      kfree(new_ilm);
      *status = -ENOMEM;
      return NULL;
    }
  memset (new_ilm->nhlfe_list, 0, sizeof (struct nhlfe_entry));

  /* Create an empty outgoing label for this nhlfe */
  new_ilm->nhlfe_list->primitive =
    (struct label_primitive *)kmalloc(sizeof(struct label_primitive),
                                      GFP_ATOMIC);
  if(new_ilm->nhlfe_list->primitive == NULL)
    {
      kfree(new_ilm->nhlfe_list);
      kfree(new_ilm);
      *status = -ENOMEM;
      return NULL;
    }
  memset (new_ilm->nhlfe_list->primitive, 0, sizeof (struct label_primitive));

  /* Set status to success */
  *status = SUCCESS;

  /* Set the ilm in the info */
  tn->info = new_ilm;

  /* Return new ftn */
  return new_ilm;
}
