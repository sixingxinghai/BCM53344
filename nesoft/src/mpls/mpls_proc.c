/* Copyright (C) 2001-2003  All Rights Reserved.  */

/* /proc file system interface.  */
 
#include "mpls_fwd.h"

#include "mpls_common.h"
#include "mpls_client.h"

#include "mpls_fib.h"
#include "mpls_hash.h"
#include "mpls_table.h"
#include "fncdef.h"

extern struct mpls_hash if_hash;

struct net_device *dev_get_by_index(struct net *net, int ifindex);

/* Opcode to string conversion array.  */
char *str_opcode[] = 
{
  "N/A",
  "PUSH",
  "POP",
  "SWAP",
  "POP FOR VPN",
  "DELIVER TO IP FOR VPN",
  "PUSH AND LOOKUP",
  "PUSH_FOR_VC",
  "PUSH_AND_LOOKUP_FOR_VC",
  "POP_FOR_VC",
  "SWAP_AND_LOOKUP",
  "NO OP",
  "FTN_LOOKUP"
};

extern struct fib_handle *fib_handle;

#define MPLS_PROC_OUTPUT_HELPER(t)      \
  do {                                  \
    len += size;                        \
    pos += size;                        \
    if (pos < offset)                   \
      {                                 \
        len = 0;                        \
        begin = pos;                    \
      }                                 \
    if (pos > offset + length)          \
      {                                 \
        if (t)                          \
          mpls_table_unlock_node (t);   \
        goto done;                      \
      }                                 \
  } while (0)

#define MPLS_PROC_DONE_HELPER()         \
  do {                                  \
    *start = buffer + (offset - begin); \
    len -= (offset - begin);            \
    if (len > length)                   \
    len = length;                       \
    if (len < 0)                        \
      len = 0;                          \
    } while (0)

/* The procfs interface functions.  */
static int
mpls_ilm_procinfo (struct seq_file *m, void *v) 
{
  struct mpls_table_node *tn;
  struct mpls_table *ilm_table;
  struct ilm_entry *ilmp;
  struct net_device *in_dev;

  /* Print data from all the ILM tables. */
  for (ilm_table = fib_handle->ilm_list; ilm_table;
       ilm_table = ilm_table->next)
    {
      seq_printf (m,
                  "\n\nDetails for ILM table bound to labelspace: %d --->\n",
                  ilm_table->ident);      
      
      for (tn = mpls_table_top (ilm_table); tn; tn = mpls_table_next (tn))
        {
          /* Only proceed for a valid route node */
          if (! tn->info)
            continue;
          
          ilmp = tn->info;
          MPLS_ASSERT (ilmp->nhlfe_list->rt != NULL);
          
          if (ilmp->nhlfe_list->primitive->opcode == POP_FOR_VC)
            {
              in_dev = dev_get_by_index (&init_net, ilmp->inl2id);
              seq_printf (m,
                          " Incoming label %d with incoming interface %s "
                          "and outgoing interface %s.\n"
                          "  Virtual Circuit Id is %d, with nexthop "
                          "%d.%d.%d.%d and opcode is %s.\n",
                          (int)ilmp->label,
                          in_dev->name,
                          ilmp->nhlfe_list->rt->u.dst.dev->name,
                          ntohl (ilmp->fec.u.prefix4.s_addr),
                          NIPQUAD (ilmp->nhlfe_list->rt->rt_gateway),
                          str_opcode[ilmp->nhlfe_list->primitive->opcode]);
            }
          else
            {
              in_dev = dev_get_by_index (&init_net, ilmp->inl2id);
              seq_printf (m,
                          " %d --> %d for FEC %d.%d.%d.%d/%d, "
                          "with nexthop %d.%d.%d.%d.\n"
                          "  Outgoing interface is %s, and opcode is %s "
                          " Incoming interface is %s\n",
                          (int)ilmp->label,
                          (int)ilmp->nhlfe_list->primitive->label,
                          NIPQUAD (ilmp->fec.u.prefix4.s_addr),
                          ilmp->fec.prefixlen,
                          NIPQUAD (ilmp->nhlfe_list->rt->rt_gateway),
                          ilmp->nhlfe_list->rt->u.dst.dev->name,
                          str_opcode[ilmp->nhlfe_list->primitive->opcode],
                          (in_dev ? in_dev->name: "-"));
            }
        }
    }

  return 0;
}

static int
mpls_ftn_procinfo (struct seq_file *m, void *v) 
{
  struct mpls_table_node *tn;
  struct ftn_entry *ftnp;

  /* Here we do the actual FTN table read. */
  for (tn = mpls_table_top (fib_handle->ftn); tn; tn = mpls_table_next (tn))
    {
      /* Only proceed for a valid route node */
      if (! tn->info)
        continue;
      
      ftnp = tn->info;

      while (ftnp)
        {
	  MPLS_ASSERT (ftnp->nhlfe_list->rt != NULL);

	  /* Not suited for printing multiple label/opcode entries
	     bound to a single ILM entry.  */
	  seq_printf(m,
	 	     " [%d.%d.%d.%d/%d] ---> %d, with outgoing interface %s.\n"
		     "  Nexthop address is %d.%d.%d.%d with opcode %s\n",
		     NIPQUAD (ftnp->fec.u.prefix4.s_addr),
		     (int)ftnp->fec.prefixlen,
		     (int)ftnp->nhlfe_list->primitive->label,
		     ftnp->nhlfe_list->rt->u.dst.dev->name,
		     NIPQUAD(ftnp->nhlfe_list->rt->rt_gateway),
		     str_opcode[ftnp->nhlfe_list->primitive->opcode]);

	  ftnp = ftnp->next;
	}
    }
  
  return 0;
}

static int
mpls_vrf_procinfo (struct seq_file *m, void *v) 
{
  struct mpls_table_node *tn;
  struct mpls_table *vrf;
  struct ftn_entry *ftnp;

  /* Make sure that the vrf list exists */
  if (! fib_handle->vrf_list)
    return 0;

  /* Go through all the vrf tables */
  for (vrf = fib_handle->vrf_list; vrf; vrf = vrf->next)
    {
      /* Print VRF header */
      seq_printf (m, "\nData for VRF Table # %d --->\n", vrf->ident);

      /* Here we do the actual FTN table read. */
      for (tn = mpls_table_top (vrf); tn; tn = mpls_table_next (tn))
        {
          /* Only proceed for a valid route node */
          if (! tn->info)
            continue;

          ftnp = tn->info;

	  while (ftnp)
            {
	      MPLS_ASSERT (ftnp->nhlfe_list->rt != NULL)
          
		/* Not suited for printing multiple label/opcode entries
		   bound to a single ILM entry.  */
		seq_printf(m,
			" [%d.%d.%d.%d/%d] ---> %d, with outgoing interface %s.\n"
			"  Nexthop address is %d.%d.%d.%d with opcode %s\n",
			NIPQUAD (ftnp->fec.u.prefix4.s_addr),
			(int)ftnp->fec.prefixlen,
			(int)ftnp->nhlfe_list->primitive->label,
			ftnp->nhlfe_list->rt->u.dst.dev->name,
			NIPQUAD(ftnp->nhlfe_list->rt->rt_gateway),
			str_opcode[ftnp->nhlfe_list->primitive->opcode]);

	      ftnp = ftnp->next;
	    }
        }
    }
  
  return 0;
}

static int
mpls_labelspace_procinfo (struct seq_file *m, void *v)
{
  struct mpls_interface *ifp;
  struct net_device *dev;
  struct mpls_hash_bucket *hb;
  int i;

  for (i = 0; i < if_hash.size; i++)
    for (hb = if_hash.index [i]; hb; hb = hb->next)
      {
        ifp = (struct mpls_interface *)hb->data;
        if (ifp)
          {
            dev = dev_get_by_index (&init_net, (int) ifp->ifindex);
                
            /* Now print the name, and the label space */
            seq_printf (m,
                        " Interface %s with label space %d is in %s "
                        "mode.\n",
                        dev->name,
                        (int)ifp->label_space,
                         (ifp->status == MPLS_ENABLED ?
                         "Enabled" : "Disabled"));
          }
      }

  return 0;
}

/* Virtual Circuit data in PROC file system. */
static int
mpls_vc_procinfo (struct seq_file *m, void *v) 
{
  struct mpls_interface *ifp;
  struct label_primitive *prim;
  uint32 nhop;
  struct net_device *dev;
  struct mpls_hash_bucket *hb;
  int i;

  for (i = 0; i < if_hash.size; i++)
    for (hb = if_hash.index [i]; hb; hb = hb->next)
      {
        ifp = (struct mpls_interface *)hb->data;
        if (ifp)
          {
            /* If VC_ID is invalid, skip this interface. */
            if (ifp->vc_id == VC_INVALID)
              continue;

            dev = dev_get_by_index (&init_net, (int) ifp->ifindex);

            if (ifp->vc_ftn)
              {
                prim = ifp->vc_ftn->nhlfe_list->primitive;
                nhop =
                  ifp->vc_ftn->nhlfe_list->rt->rt_gateway;
              }
            else
              {
                prim = NULL;
                nhop = 0;
              }
                
            /* Print interface details. */
            seq_printf (m,
                        " Interface %s bound to Virtual Circuit ID %d with opcode %s.\n"
                        "  Outgoing label is %d with nexthop address %d.%d.%d.%d\n",
                        dev->name, (int)ifp->vc_id,
                        ((! prim) ? "N/A" :
                         str_opcode [(int)prim->opcode]),
                        ((! prim) ? -1 : (int)prim->label),
                        NIPQUAD (nhop));
          }
      }

  return 0;
}

/* Virtual Circuit data in PROC file system. */
static int
mpls_stats_procinfo (struct seq_file *m, void *v) 
{
  seq_printf (m,
              " TX labeled IP packets: %lu\n",
              fib_handle->stats.labeled_tx_pkts);

  seq_printf (m,
              " TX packets to IP: %lu\n", fib_handle->stats.ip_tx_pkts);

  seq_printf (m,
              " TX switched packets: %lu\n",
              fib_handle->stats.switched_pkts);

  seq_printf (m,
              " Dropped packets: %lu\n",
              fib_handle->stats.dropped_pkts);

  seq_printf (m,
              " Dropped labeled packets: %lu\n",
              fib_handle->stats.dropped_labeled_pkts);

  return 0;
}

static int
mpls_if_stats_procinfo (struct seq_file *m, void *v)
{
  struct mpls_interface *ifp;
  struct mpls_hash_bucket *hb;
  int i;

  seq_printf (m,
              "Ifindex   "
              "RxPkts      "
              "DroppedPkts "
              "IPTxPkts    "
              "LabelTxPkts "
              "DropLbPkts  "
              "SwtTxPkts   "
              "LbLkupFail  "
              "OutFragment\n");

  /* Now print the name, and the label space */
  seq_printf (m,
              "%-10d%-12lu%-12lu%-12lu%-12lu%-12lu%-12lu%-12lu%-12lu\n",
              0,
              fib_handle->if_stats.rx_pkts,
              fib_handle->if_stats.dropped_pkts,
              fib_handle->if_stats.ip_tx_pkts,
              fib_handle->if_stats.labeled_tx_pkts,
              fib_handle->if_stats.dropped_labeled_pkts,
              fib_handle->if_stats.switched_pkts,
              fib_handle->if_stats.label_lookup_failures,
              fib_handle->if_stats.out_fragments);

  for (i = 1; i < if_hash.size; i++)
    for (hb = if_hash.index [i]; hb; hb = hb->next)
      {
        ifp = (struct mpls_interface *)hb->data;
        if (ifp)
          {
            /* Now print the name, and the label space */
            seq_printf (m,
                        "%-10lu%-12lu%-12lu%-12lu%-12lu%-12lu%-12lu%-12lu%-12lu\n",
                        ifp->ifindex,
                        ifp->stats.rx_pkts,
                        ifp->stats.dropped_pkts,
                        ifp->stats.ip_tx_pkts,
                        ifp->stats.labeled_tx_pkts,
                        ifp->stats.dropped_labeled_pkts,
                        ifp->stats.switched_pkts,
                        ifp->stats.label_lookup_failures,
                        ifp->stats.out_fragments);
          }
      }

  return 0;
}

static int
mpls_ilm_stats_procinfo (struct seq_file *m, void *v) 
{
  struct mpls_table_node *tn;
  struct mpls_table *ilm_table;
  struct ilm_entry *ilmp;
  uint8 heading = 0;
  char dummy[17];

  for (ilm_table = fib_handle->ilm_list;
       ilm_table;
       ilm_table = ilm_table->next)
    for (tn = mpls_table_top (ilm_table); tn; tn = mpls_table_next (tn))
      if ((ilmp = tn->info) != NULL)
        {
          MPLS_ASSERT (ilmp->nhlfe_list->rt != NULL);

          if (heading == 0)
            {
              heading = 1;
              seq_printf (m,
                          "InIfindex:Label   "
                          "RxBytes     "
                          "TxBytes     "
                          "RxPkts      "
                          "SwappedPkts "
                          "PoppedPkts\n");
            }
          
          memset (dummy, 0, 17);
          sprintf(dummy, "%lu:%lu", ilmp->inl2id, ilmp->label);
          seq_printf (m,
                      "%-18s%-12lu%-12lu%-12lu%-12lu%-12lu\n",
                      dummy,
                      ilmp->stats.rx_bytes,
                      ilmp->stats.tx_bytes,
                      ilmp->stats.rx_pkts,
                      ilmp->stats.swapped_pkts,
                      ilmp->stats.popped_pkts);
        }
  
  return 0;
}

static int
mpls_ftn_stats_procinfo (struct seq_file *m, void *v) 
{
  struct mpls_table *vrf;
  struct mpls_table_node *tn;
  struct ftn_entry *ftnp;
  int ident;
  char dummy[20];
  uint8 heading = 0;

  for (vrf = fib_handle->ftn;
       vrf;
       vrf = (vrf == fib_handle->ftn ? fib_handle->vrf_list : vrf->next))
    {
      /* Show GLOBAL TABLE as zero. */
      ident = (vrf->ident == -1 ? 0 : vrf->ident);
      
      for (tn = mpls_table_top (vrf); tn; tn = mpls_table_next (tn))
        if ((ftnp = tn->info) != NULL)
          {
            MPLS_ASSERT (ftnp->nhlfe_list->rt != NULL);
            
            if (heading == 0)
              {
                heading = 1;
                seq_printf (m,
                            "VRF-ID   "
                            "A.B.C.D/M           "
                            "TxBytes     "
                            "PushedPkts  "
                            "MatchedBytes   "
                            "MatchedPkts\n");
              }
            
            memset (dummy, 0, 20);
            sprintf (dummy, "%d.%d.%d.%d/%d",
                     NIPQUAD (ftnp->fec.u.prefix4.s_addr),
                     (int)ftnp->fec.prefixlen);

            seq_printf(m,
                       "%-9d%-20s%-12lu%-12lu%-15lu%-12lu\n",
                       ident,
                       dummy,
                       ftnp->stats.tx_bytes,
                       ftnp->stats.pushed_pkts,
                       ftnp->stats.matched_bytes,
                       ftnp->stats.matched_pkts);
          }
    }
  
  return 0;
}

static int
mpls_nhlfe_stats_procinfo (struct seq_file *m, void *v)
{
  struct mpls_table *vrf;
  struct mpls_table_node *tn;
  struct ftn_entry *ftnp;
  struct mpls_table *ilm_table;
  struct ilm_entry *ilmp;
  uint8 heading = 0;

  for (vrf = fib_handle->ftn;
       vrf;
       vrf = (vrf == fib_handle->ftn ? fib_handle->vrf_list : vrf->next))
    {
      for (tn = mpls_table_top (vrf); tn; tn = mpls_table_next (tn))
        if ((ftnp = tn->info) != NULL)
          {
            MPLS_ASSERT (ftnp->nhlfe_list->rt != NULL);
            
            if (heading == 0)
              {
                heading = 1;
                seq_printf (m,
                            "NHLFE-IX   "
                            "TxBytes     "
                            "TxPkts      "
                            "ErrorPkts   "
                            "DiscardPkts\n");
              }
            
            seq_printf(m,
                       "%-11lu%-12lu%-12lu%-12lu%-12lu\n",
                       ftnp->nhlfe_list->nhlfe_ix,
                       ftnp->nhlfe_list->stats.tx_bytes,
                       ftnp->nhlfe_list->stats.tx_pkts,
                       ftnp->nhlfe_list->stats.error_pkts,
                       ftnp->nhlfe_list->stats.discard_pkts);
          }
    }
  
  for (ilm_table = fib_handle->ilm_list;
       ilm_table;
       ilm_table = ilm_table->next)
    for (tn = mpls_table_top (ilm_table); tn; tn = mpls_table_next (tn))
      if ((ilmp = tn->info) != NULL)
        {
          MPLS_ASSERT (ilmp->nhlfe_list->rt != NULL);

            if (heading == 0)
              {
                heading = 1;
                seq_printf (m,
                            "NHLFE-IX   "
                            "TxBytes     "
                            "TxPkts      "
                            "ErrorPkts   "
                            "DiscardPkts\n");
              }
            
            seq_printf(m,
                       "%-11lu%-12lu%-12lu%-12lu%-12lu\n",
                       ilmp->nhlfe_list->nhlfe_ix,
                       ilmp->nhlfe_list->stats.tx_bytes,
                       ilmp->nhlfe_list->stats.tx_pkts,
                       ilmp->nhlfe_list->stats.error_pkts,
                       ilmp->nhlfe_list->stats.discard_pkts);
        }
  
  return 0;
}

static int
mpls_tunnel_stats_procinfo (struct seq_file *m, void *v)
{
  struct mpls_table *vrf;
  struct mpls_table_node *tn;
  struct ftn_entry *ftnp;
  struct mpls_table *ilm_table;
  struct ilm_entry *ilmp;
  uint8 heading = 0;

  for (vrf = fib_handle->ftn;
       vrf;
       vrf = (vrf == fib_handle->ftn ? fib_handle->vrf_list : vrf->next))
    {
      for (tn = mpls_table_top (vrf); tn; tn = mpls_table_next (tn))
        if ((ftnp = tn->info) != NULL)
          if (ftnp->owner.protocol == APN_PROTO_RSVP &&
              ftnp->owner.u.r_key.u.ipv4.trunk_id)
            {
              MPLS_ASSERT (ftnp->nhlfe_list->rt != NULL);
            
              if (heading == 0)
                {
                  heading = 1;
                  seq_printf (m,
                              "Trunk ID  "
                              "LSP ID    "
                              "Ingr ID    "
                              "Egr ID     "
                              "TxBytes     "
                              "TxPkts      "
                              "ErrorPkts\n");
                }
            
              seq_printf(m,
                         "%-10d%-10d%08x   %08x   %-12lu%-12lu%-12lu\n",
                         ftnp->owner.u.r_key.u.ipv4.trunk_id,
                         ftnp->owner.u.r_key.u.ipv4.lsp_id,
                         ftnp->owner.u.r_key.u.ipv4.ingr.s_addr,
                         ftnp->owner.u.r_key.u.ipv4.egr.s_addr,
                         ftnp->nhlfe_list->stats.tx_bytes,
                         ftnp->nhlfe_list->stats.tx_pkts,
                         (ftnp->nhlfe_list->stats.error_pkts +
                          ftnp->nhlfe_list->stats.discard_pkts));
            }
    }
  
  for (ilm_table = fib_handle->ilm_list;
       ilm_table;
       ilm_table = ilm_table->next)
    for (tn = mpls_table_top (ilm_table); tn; tn = mpls_table_next (tn))
      if ((ilmp = tn->info) != NULL)
        if (ilmp->owner.protocol == APN_PROTO_RSVP &&
            ilmp->owner.u.r_key.u.ipv4.trunk_id)
          {
            MPLS_ASSERT (ilmp->nhlfe_list->rt != NULL);

              if (heading == 0)
                {
                  heading = 1;
                  seq_printf (m,
                              "Trunk ID  "
                              "LSP ID    "
                              "Ingr ID    "
                              "Egr ID     "
                              "TxBytes     "
                              "TxPkts      "
                              "ErrorPkts\n");
                }
            
              seq_printf(m,
                         "%-10d%-10d%08x   %08x   %-12lu%-12lu%-12lu\n",
                         ilmp->owner.u.r_key.u.ipv4.trunk_id,
                         ilmp->owner.u.r_key.u.ipv4.lsp_id,
                         ilmp->owner.u.r_key.u.ipv4.ingr.s_addr,
                         ilmp->owner.u.r_key.u.ipv4.egr.s_addr,
                         ilmp->nhlfe_list->stats.tx_bytes,
                         ilmp->nhlfe_list->stats.tx_pkts,
                         (ilmp->nhlfe_list->stats.error_pkts +
                          ilmp->nhlfe_list->stats.discard_pkts));
          }
  
  return 0;
}

static int 
mpls_ilm_proc_open(struct inode *inode, struct file *file)
{
  return single_open (file, mpls_ilm_procinfo, NULL);
}

static const struct file_operations mpls_ilm_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= mpls_ilm_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

struct proc_dir_entry *
mpls_proc_net_create_ilm()
{
  struct proc_dir_entry *res;
  res = proc_create ("mpls_ilm", 0, init_net.proc_net, &mpls_ilm_proc_fops); 
  return res;
}

static int 
mpls_ftn_proc_open(struct inode *inode, struct file *file)
{
  return single_open (file, mpls_ftn_procinfo, NULL);
}

static const struct file_operations mpls_ftn_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= mpls_ftn_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

struct proc_dir_entry *
mpls_proc_net_create_ftn()
{
  struct proc_dir_entry *res;
  res = proc_create ("mpls_ftn", 0, init_net.proc_net, &mpls_ftn_proc_fops);
  return res;
}

static int 
mpls_vrf_proc_open(struct inode *inode, struct file *file)
{
  return single_open (file, mpls_vrf_procinfo, NULL);
}

static const struct file_operations mpls_vrf_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= mpls_vrf_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

struct proc_dir_entry *
mpls_proc_net_create_vrf()
{
  struct proc_dir_entry *res;
  res = proc_create ("mpls_vrf", 0, init_net.proc_net, &mpls_vrf_proc_fops);
  return res;
}

static int 
mpls_labelspace_proc_open(struct inode *inode, struct file *file)
{
  return single_open (file, mpls_labelspace_procinfo, NULL);
}

static const struct file_operations mpls_labelspace_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= mpls_labelspace_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

struct proc_dir_entry *
mpls_proc_net_create_labelspace()
{
  struct proc_dir_entry *res;
  res = proc_create ("mpls_labelspace", 0, init_net.proc_net, &mpls_labelspace_proc_fops);
  return res;
}

static int 
mpls_vc_proc_open(struct inode *inode, struct file *file)
{
  return single_open (file, mpls_vc_procinfo, NULL);
}

static const struct file_operations mpls_vc_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= mpls_vc_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

struct proc_dir_entry *
mpls_proc_net_create_vc()
{
  struct proc_dir_entry *res;
  res = proc_create ("mpls_vc", 0, init_net.proc_net, &mpls_vc_proc_fops);
  return res;
}

static int 
mpls_stats_proc_open(struct inode *inode, struct file *file)
{
  return single_open (file, mpls_stats_procinfo, NULL);
}

static const struct file_operations mpls_stats_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= mpls_stats_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

struct proc_dir_entry *
mpls_proc_net_create_stats()
{
  struct proc_dir_entry *res;
  res = proc_create ("mpls_stats", 0, init_net.proc_net, &mpls_stats_proc_fops);
  return res;
}

static int 
mpls_ftn_stats_proc_open(struct inode *inode, struct file *file)
{
  return single_open (file, mpls_ftn_stats_procinfo, NULL);
}

static const struct file_operations mpls_ftn_stats_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= mpls_ftn_stats_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

struct proc_dir_entry *
mpls_proc_net_create_ftn_stats()
{
  struct proc_dir_entry *res;
  res = proc_create ("mpls_ftn_stats", 0, init_net.proc_net, &mpls_ftn_stats_proc_fops);
  return res;
}

static int 
mpls_ilm_stats_proc_open(struct inode *inode, struct file *file)
{
  return single_open (file, mpls_ilm_stats_procinfo, NULL);
}

static const struct file_operations mpls_ilm_stats_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= mpls_ilm_stats_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

struct proc_dir_entry *
mpls_proc_net_create_ilm_stats()
{
  struct proc_dir_entry *res;
  res = proc_create ("mpls_ilm_stats", 0, init_net.proc_net, &mpls_ilm_stats_proc_fops);
  return res;
}

static int 
mpls_nhlfe_stats_proc_open(struct inode *inode, struct file *file)
{
  return single_open (file, mpls_nhlfe_stats_procinfo, NULL);
}

static const struct file_operations mpls_nhlfe_stats_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= mpls_nhlfe_stats_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

struct proc_dir_entry *
mpls_proc_net_create_nhlfe_stats()
{
  struct proc_dir_entry *res;
  res = proc_create ("mpls_nhlfe_stats", 0, init_net.proc_net, &mpls_nhlfe_stats_proc_fops);
  return res;
}

static int 
mpls_tunnel_stats_proc_open(struct inode *inode, struct file *file)
{
  return single_open (file, mpls_tunnel_stats_procinfo, NULL);
}

static const struct file_operations mpls_tunnel_stats_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= mpls_tunnel_stats_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

struct proc_dir_entry *
mpls_proc_net_create_tunnel_stats()
{
  struct proc_dir_entry *res;
  res = proc_create ("mpls_tunnel_stats", 0, init_net.proc_net, 
                     &mpls_tunnel_stats_proc_fops);
  return res;
}

static int 
mpls_if_stats_proc_open(struct inode *inode, struct file *file)
{
  return single_open (file, mpls_if_stats_procinfo, NULL);
}

static const struct file_operations mpls_if_stats_proc_fops = {
	.owner		= THIS_MODULE,
	.open		= mpls_if_stats_proc_open,
	.read		= seq_read,
	.llseek		= seq_lseek,
	.release	= single_release,
};

struct proc_dir_entry *
mpls_proc_net_create_if_stats()
{
  struct proc_dir_entry *res;
  res = proc_create ("mpls_if_stats", 0, init_net.proc_net, &mpls_if_stats_proc_fops);
  return res;
}

/* End Of File */
