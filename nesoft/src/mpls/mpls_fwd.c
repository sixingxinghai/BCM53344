/* Copyright (C) 2001-2003  All Rights Reserved.  */

#include "mpls_fwd.h"

#include "mpls_common.h"
#include "mpls_client.h"

#include "mpls_fib.h"
#include "mpls_hash.h"
#include "mpls_table.h"
#include "fncdef.h"

/* Global variables.  */
u32 milds = 1488;
u32 router_alert = 0;
u32 max_frag_size = 0;

/* Set the default ttl values here */
int ingress_ttl = -1;
int egress_ttl = -1;
uint8 propagate_ttl = TTL_PROPAGATE_ENABLED;
uint8 lsp_model     = LSP_MODEL_UNIFORM;

/* Flag to enable/disable handling of locally generated TCP packets */
uint8 local_pkt_handle = 0;

/* Flag to indicate whether debug information should be printed or not */
uint8 show_error_msg = 0;    /* Off by default */
uint8 show_warning_msg = 0;  /* Off by default */
uint8 show_debug_msg = 0;    /* Off by default */
uint8 show_notice_msg = 0;   /* Off by default */

/* Type of the module parameter.  */
module_param(milds, int, 0644);

/* These are macros helpfully provided for us. The utility modinfo can
   be used to examine this stuff.  */
MODULE_DESCRIPTION("MPLS Forwarder");
MODULE_LICENSE ("APN MPLS Forwarder"); 


/*---------------------------------------------------------------------------
  We fill the following packet structure and register it with the kernel so
  that the kernel recognizes packets of ethernet type ETH_P_MPLS_UC as valid
  packets and calls our receive handler each time a packet is received.
  ---------------------------------------------------------------------------*/
static struct packet_type mpls_uc_pkt = 
  {
    /* Packet type that we're interested in (MPLS unicast packets) */
    /* type */
    .type = __constant_htons(ETH_P_MPLS_UC),

    /* We're not device-specific. Irrespective of the incoming
       interface we are ready to accept an ethernet packet which has a
       type of MPLS unicast in the 'type' field of the ethernet
       header. NULL acts as a wildcard for devices.  */
    /* dev */
    .dev = NULL,                                

    /* Func which does receive processing. For every ethernet frame
       having a type field of ETH_P_MPLS_UC, the kernel will call
       mpls_rcv with the following arguments in the specified order
       1. struct sk_buff* sk 2. struct net_device* dev 3. struct
       packet_type* pt where 'sk' contains the full packet received,
       'dev' is a pointer to the net_device structure over which the
       packet arrived and 'pt' is the packet type structure.  NOTE:
       'pt' is not used in any way in further processing. Its simply
       ignored.  */
    /* func */
    .func = mpls_rcv,                            

    /* Private data for the packet. Not used.  */
    /* data */
    .af_packet_priv = NULL,

    /* next, prev */
    .list = {NULL, NULL}
  };

/* Even IP packets should be passed to the MPLS forwarder.  */
static struct packet_type ip_pkt = 
  {
    .type = __constant_htons(ETH_P_IP),
    .dev = NULL,                                /* All devices */
    .func = mpls_rcv,
    .af_packet_priv = NULL,
    /*
     * if we make the data part as non-zero the kernel will assume 
     * that we can handle shared skb's and simply pass the skb to our
     * handler, else it clones the skb and then passes it
     */
    /*(void *)1, */
    /* next, prev */
    .list = {NULL, NULL}
  };

/* Pkt handler for ARP */
static struct packet_type arp_pkt = 
  {
    .type =__constant_htons(ETH_P_ARP),
    .dev = NULL,
    .func = mpls_rcv,
    .af_packet_priv = NULL,
    .list = {NULL, NULL}
  };


/* Pointer to keep a copy of the IP packet handler */
static struct packet_type *orig_ip_pkt = NULL;

/* This function is called once during startup. It registers itself to
  receive packets of type ETH_P_MPLS_UC and sets up the four entries
  in /proc/net and sets routine to allow access to those /proc/net/
  files.  */
static int __init
mpls_init (void)
{
  struct sock *sock;
  int ret;

  /* Netlink socket to read requests from the userspace protocols */
  sock = mpls_netlink_sock_create ();
  if (sock == NULL)
    {
      PRINTK_ERR ("Error in creating netlink socket\n");
      return FAILURE;
    }

  /* IP packets also should be processed by the MPLS module. */
  dev_add_pack(&ip_pkt);

  /* Register ourselves to receive incoming MPLS packets. */
  dev_add_pack(&mpls_uc_pkt);

  /* Register handler for ARP packets */
  dev_add_pack (&arp_pkt);

  /* Remove the IP handler */
  ret = remove_handler ();
  if (ret < 0)
    PRINTK_ERR ("Error : Could not remove IP Handler\n");

#if CONFIG_PROC_FS
  /* /proc interface to the MPLS related data structures A note on
     'proc_net_create': First argument is the name by which it will
     appear in the /proc filesystem, second argument is the mode of
     the file (0 means it will be a readonly file), third argument is
     the function which will get called when we do a "read" on the
     particular proc file.  */

  /* Show all ILM table entries. */
  if (mpls_proc_net_create_ilm() == NULL)
    {
      PRINTK_WARNING ("mpls_init : Cannot create /proc/net "
                      "entry for mpls_ilm.\n");
      return FAILURE;
    }

  /* Show all FTN table entries.  */
  if (mpls_proc_net_create_ftn() == NULL)
    {
      PRINTK_WARNING ("mpls_init : Cannot create /proc/net "
                      "entry for mpls_ftn.\n");
      return FAILURE;
    }

  /* Show all VRF tables */
  if (mpls_proc_net_create_vrf() == NULL)
    {
      PRINTK_WARNING ("mpls_init : Cannot create /proc/net "
                      "entry for mpls_vrf.\n");
      return FAILURE;
    }


  /* Show all MPLS enabled interfaces */
  if (mpls_proc_net_create_labelspace() == NULL)
    {
      PRINTK_WARNING ("mpls_init : Cannot create /proc/net "
                      "entry for mpls_labelspace.\n");
      return FAILURE;
    }

  /* Show Virtual Circuit data. */
  if (mpls_proc_net_create_vc () == NULL)
    {
      PRINTK_WARNING ("mpls_init : Cannot create /proc/net "
                      "entry for mpls_vc.\n");
      return FAILURE;
    }

  /* Show top level statistics. */
  if (mpls_proc_net_create_stats () == NULL)
    {
      PRINTK_WARNING ("mpls_init : Cannot create /proc/net "
                     "entry for mpls_stats.\n");
      return FAILURE;
    }
  
  /* Show FTN statistics. */
  if (mpls_proc_net_create_ftn_stats () == NULL)
    {
      PRINTK_WARNING ("mpls_init : Cannot create /proc/net "
                      "entry for mpls_ftn_stats.\n");
      return FAILURE;
    }
  
  /* Show ILM statistics. */
  if (mpls_proc_net_create_ilm_stats () == NULL)
    {
      PRINTK_WARNING ("mpls_init : Cannot create /proc/net "
                      "entry for mpls_ilm_stats.\n");
      return FAILURE;
    }
  
  /* Show NHLFE statistics. */
  if (mpls_proc_net_create_nhlfe_stats () == NULL)
    {
      PRINTK_WARNING ("mpls_init : Cannot create /proc/net "
                      "entry for mpls_nhlfe_stats.\n");
      return FAILURE;
    }

  /* Show TE TUNNEL statistics. */
  if (mpls_proc_net_create_tunnel_stats () == NULL)
    {
      PRINTK_WARNING ("mpls_init : Cannot create /proc/net "
                      "entry for mpls_tunnel_stats.\n");
      return FAILURE;
    }
  
  /* Show interface statistics. */
  if (mpls_proc_net_create_if_stats () == NULL)
    {
      PRINTK_WARNING ("mpls_init : Cannot create /proc/net "
                     "entry for mpls_if_stats.\n");
      return FAILURE;
    }
#endif /* CONFIG_PROC_FS */

  return SUCCESS;
}

/* dst_ops structure for a labelled packet. */
struct dst_ops labelled_dst_ops =
  {
    AF_INET,
    __constant_htons(ETH_P_MPLS_UC),
    0,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    NULL,
    sizeof(struct rtable),
};

/* Initialize the MPLS module. It regsiters a handler function for
 * packets of type ETH_P_IP and ETH_P_MPLS_UC.  */
int 
init_module (void) 
{
  int ret; 

  /* Initialize the tables etc. */
  ret = mpls_initialize_fib_handle ();
  if (ret != SUCCESS)
    return ret;

  if (mpls_init() == FAILURE)
    {
      PRINTK_ERR ("init_module : "
                  "module not successfully installed.\n");
      return FAILURE;
    }

  if (local_pkt_handle)
    mpls_register_output_hdlr (mpls_tcp_output);

  PRINTK_NOTICE ("Successfully initialized MPLS Module\n");
  
  return SUCCESS;
}

/* Cleanup - unregister the appropriate file from /proc.  */
void
cleanup_module()
{
  if (local_pkt_handle)
    mpls_unregister_output_hdlr ();

  mpls_cleanup_ok();

  /* The IP Handler was removed from the handler list earlier */
  /* Put it back */
  if (orig_ip_pkt)
    dev_add_pack (orig_ip_pkt);

  /* Return the system state back to normal */
  dev_remove_pack(&mpls_uc_pkt);
  dev_remove_pack(&ip_pkt);
  dev_remove_pack (&arp_pkt);

  /* Close the netlink socket */
  mpls_netlink_sock_close ();

#if CONFIG_PROC_FS
  remove_proc_entry("mpls_ilm", init_net.proc_net); 
  remove_proc_entry("mpls_ftn", init_net.proc_net);
  remove_proc_entry("mpls_vrf", init_net.proc_net);
  remove_proc_entry("mpls_labelspace", init_net.proc_net);
  remove_proc_entry("mpls_vc", init_net.proc_net );
  remove_proc_entry("mpls_stats", init_net.proc_net);
  remove_proc_entry("mpls_ftn_stats", init_net.proc_net);
  remove_proc_entry("mpls_ilm_stats", init_net.proc_net);
  remove_proc_entry("mpls_nhlfe_stats", init_net.proc_net);
  remove_proc_entry("mpls_tunnel_stats", init_net.proc_net);
  remove_proc_entry("mpls_if_stats", init_net.proc_net); 
#endif /* CONFIG_PROC_FS */

  rt_purge_list (1);
  
  /* Destroy everything */
  mpls_destroy_fib_handle ();

  PRINTK_NOTICE ("Successfully removed MPLS Module\n");
}

/*
 * Remove the specified function from the list of handlers to be
 * called for a received packet.
 *
 * This assumes that our handler was added before this routine
 * is called.
 */
int
remove_handler ()
{
  struct packet_type *node, *ip_node = NULL;

  list_for_each_entry(node, &ip_pkt.list, list)
    {
      /* Find the default IP packet handler */
      if (node->func == ip_rcv)
        {
          ip_node = node;
          break;
        }
    }

  if (ip_node == NULL)
    list_for_each_entry_reverse (node, &ip_pkt.list, list)
      {
        /* Find the default IP packet handler */
        if (node->func == ip_rcv)
          {
            ip_node = node;
            break;
          }
      }

  if (ip_node == NULL)
    return -1;

  orig_ip_pkt = ip_node;
  dev_remove_pack (orig_ip_pkt);
  
  return 0;
}
