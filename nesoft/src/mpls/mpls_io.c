/* Copyright (C) 2001-2003  All Rights Reserved.  */

#include "mpls_fwd.h"

#include "mpls_common.h"
#include "mpls_client.h"

#include "forwarder.h"
#include "mpls_fib.h"
#include "mpls_hash.h"
#include "mpls_table.h"
#include "fncdef.h"

/* #define MPLS_IPV6 */

/* Some extern declarations not covered in the above header files. */
extern struct net_device *dev_get_by_name(struct net *net, const char *name);
extern struct sk_buff *skb_copy_expand(const struct sk_buff *skb,
                                       int newheadroom, int newtailroom,
                                       gfp_t priority);
extern irq_cpustat_t irq_stat[NR_CPUS];
extern struct fib_handle *fib_handle;
extern int ip_rcv (struct sk_buff *skb, struct net_device *dev,
		   struct packet_type *pt, struct net_device *orig_dev);

#if 0
extern int (*mpls_dst_input) (struct sk_buff *);
#endif 
extern struct dst_ops *dst_ops;
extern struct dst_ops labelled_dst_ops;


/*
 * the receive handler for labelled and unlabelled packets in an MPLS capable
 * node. By the time this handler is called the relevant fields in the 
 * sk_buff structure look as follows.
 */

/*---------------------------------------------------------------------------
 * ________________________________________________________________
 * |          |      |                              |               |
 * |          | MAC  |                              |               |
 * | headroom |header|                              |  tailroom     |
 * |__________|______|______________________________|_______________|
 * 
 * A          B      C                              
 *                   
 * A : skb->head
 * B : skb->mac.raw
 * C : skb->data, skb->nh.raw, skb->h.raw
 * C can be either the start of layer 3 header or the start of the label stack,
 * depending on whether the packet is an unlabelled or a labelled one.
 ---------------------------------------------------------------------------*/

int
mpls_rcv (struct sk_buff *sk, struct net_device *dev, struct packet_type *pt,
          struct net_device *orig_dev)
{
  struct mpls_interface *ifp;
  int result;

  /*
   * if the skbuff received is "insane" in any sense, do not process it
   */
  if (!(sk && sk->data && (sk->len >= sizeof(uint32)) &&
        (sk->data - sk->head >= ETH_HLEN)))
    {
      kfree_skb(sk);
      PRINTK_ERR ("mpls_rcv : Insane sk_buff received\n");
      FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_DROPPED_PKTS, 1);
      NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_RX_PKTS, 1);
      NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_DROPPED_PKTS, 1);
      return -ENOMEM;
    }

  /* Check for VC binding. */
  ifp = mpls_if_lookup ((uint32)dev->ifindex);
  if (ifp)
    {
      IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_RX_PKTS, 1);
      if (ifp->vc_id != VC_INVALID)
        { 
          if (ifp->vc_ftn != NULL)
            return mpls_vc_rcv (sk, dev, ifp->vc_ftn, pt);

          FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_DROPPED_PKTS, 1);
          IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_PKTS, 1);

          PRINTK_NOTICE ("mpls_rcv : Received a packet on interface with "
                         "ifindex %d configured for Virtual Circuit, "
                         "and no Virtual Cirtcuit specific label. "
                         "Dropping packet\n", (int)dev->ifindex);
          kfree_skb(sk);

          return FAILURE;
        }
    }
  else
    NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_RX_PKTS, 1);

  /* If packet is not meant for us ... (Promiscuous handling) */
  if (sk->pkt_type == PACKET_OTHERHOST)
    {
      FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_DROPPED_PKTS, 1);
      if (ifp)
        IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_PKTS, 1);
      else
        NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_DROPPED_PKTS, 1);
      kfree_skb (sk);
      return (SUCCESS);
    }

  if (((struct ethhdr *)(sk->mac_header))->h_proto == __constant_htons (ETH_P_ARP))
    {
      /* Drop all arp packets */
      FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_DROPPED_PKTS, 1);
      if (ifp)
        IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_PKTS, 1);
      else
        NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_DROPPED_PKTS, 1);
      kfree_skb (sk);
      return SUCCESS;
    }

  /* If packet is a raw packet without an ethernet header, pass to ip */
  if (sk->mac_header == sk->network_header)
    {
      FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
      if (ifp)
        IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
      else
        NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS, 1);
      result = ip_rcv (sk, dev, pt, orig_dev);
      MPLS_ASSERT(result == 0);
      return (SUCCESS);
    }

  /*
   * the sk_buff we receive here has layer2 header and layer3 header
   * pointers set, the layer3 header pointer will be set wrongly for an
   * MPLS packet which the kernel does not recognize. We have to 
   * correct the pointers here as soon as we get a packet
   */
  set_proto_pointers(sk);

  /*---------------------------------------------------------------------------
   * from here on we have the layer2 and layer3 pointers set correctly.
   * shim header pointer is at a fixed offset (ETH_HLEN) from the layer2
   * header. To get the shim header pointer use
   * shimhp = (struct shimhdr *)((uint8 *)sk->mac.raw + ETH_HLEN);
   *--------------------------------------------------------------------------*/
  /*
   * loopback packets need not go thru the MPLS module. pass it to ip_rcv
   * for normal loopback processing.
   */
  if (ntohs(((struct ethhdr *)(sk->mac_header))->h_proto) == ETH_P_IP &&
      LOOPBACK (ntohl(ip_hdr(sk)->daddr)))
    {
      FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
      if (ifp)
        IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
      else
        NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS, 1);
      result = ip_rcv (sk, dev, pt, orig_dev);
      MPLS_ASSERT(result == 0);
      return (SUCCESS);
    }

  /*
   * process the packet differently based on the 'type' field in the 
   * ethernet header. It does functions like decrementing TTL and sending
   * ICMP error responses. For labelled packet it does the processing for
   * the special labels
   */
  if (l2_process(sk, dev, pt, ifp, orig_dev) == SUCCESS)
    {
      /*
       * the actual forwarding routine.
       */
      fwd_forward_frame(sk, dev, pt, ifp, orig_dev);
    }

  return (SUCCESS);
}

/*---------------------------------------------------------------------------
 * This function is called to process the sk_buff which might contain a 
 * unlabelled IP packet or a labelled MPLS packet.
 * Return value can be SUCCESS or FAILURE
 * FAILURE implies that the forwarder need not do any further processing.
 * SUCCESS implies that the forwarder should do MPLS related processing 
 * (FTN/ILM lookup etc)
 * l2_process should free the sk_buff passed on FAILURE.
 *-------------------------------------------------------------------------*/
int
l2_process(struct sk_buff *sk, struct net_device *dev, struct packet_type *pt,
           struct mpls_interface *ifp, struct net_device *orig_dev)
{
  struct shimhdr   *shimhp;
  uint32            label;
  int               retval;

  /* Reset the router_alert value */
  router_alert = 0;
 
  /*
   * act accordingly for labelled and unlabelled packet
   */
  switch (ntohs(((struct ethhdr *)(sk->mac_header))->h_proto))
    {
    case ETH_P_IP:                        /* unlabelled IP packet */
      if (ip_hdr(sk)->version == IPV6)
        {
#if defined (MPLS_IPV6) && defined (CONFIG_IPV6)
          FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
          if (ifp)
            IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
          else
            NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS, 1);
          retval = ipv6_rcv(sk, dev, pt);
#else
          FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_DROPPED_PKTS, 1);
          if (ifp)
            IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_PKTS, 1);
          else
            NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_DROPPED_PKTS, 1);
          kfree_skb(sk);    
          PRINTK_WARNING ("Panic, IPV6 packet received.\n");
#endif /* MPLS_IPV6 && CONFIG_IPV6 */
          return (FAILURE);
        }

#if 0      
      if ((mpls_dst_input == NULL) && (sk->nh.iph->saddr != 0))
        {
          u32 addr = sk->nh.iph->saddr;
          dst_data_init (addr, dev, SET_DST_ALL, RT_LOOKUP_INPUT); 
        }
#endif

      /*
       * If the destination is 255.255.255.255, just pass it
       * to IP.
       */
      if (~(ip_hdr(sk)->daddr & 0xffffffff) == 0)
        {
          FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
          if (ifp)
            IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
          else
            NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS, 1);
          retval = ip_rcv (sk, dev, pt, orig_dev);
          return (FAILURE);
        }
        
      /*
       * If this packet is a broadcast packet, send it
       * to the ip handler.
       */
      if (ip_hdr(sk)->daddr == get_if_addr (dev, MPLS_FALSE /* broadcast */))
        {
          FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
          if (ifp)
            IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
          else
            NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS, 1);
          retval = ip_rcv (sk, dev, pt, orig_dev);
          return (FAILURE);
        }

      /*
       * If this packet is a multicast packet, send it
       * to the ip handler.
       */
      if (ipv4_is_multicast (ip_hdr(sk)->daddr))
        {
          FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
          if (ifp)
            IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
          else
            NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS, 1);
          retval = ip_rcv (sk, dev, pt, orig_dev);
          return (FAILURE);
        }

      /*
       * Check whether packets needs to be labelled or not.
       */
        {
          u_char skip_labelling = 0;

          /* Do not label routing packets */
          if (ip_hdr(sk)->protocol == IPPROTO_RSVP
              || ip_hdr(sk)->protocol == IPPROTO_PIM)
            {
              skip_labelling = 1;
            }
          else if (ip_hdr(sk)->protocol == IPPROTO_UDP)
            {
              struct udphdr *udph = NULL;
              udph = (struct udphdr*)(sk->data+(ip_hdr(sk)->ihl << 2));

              /* Do not label LMP and LDP control packets */
              if ((ntohs (udph->dest) == MPLS_LDP_UDP_PORT)
                   || (ntohs (udph->dest) == MPLS_LMP_UDP_PORT))
                {
                  skip_labelling = 1;
                }
            }

          if (skip_labelling)
            {
              FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
              if (ifp)
                IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
              else
                NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS, 1);
              retval = ip_rcv (sk, dev, pt, orig_dev);
              return (FAILURE);
            }
        }

      /*
       * if the packet is destined for us or if the MPLS module
       * is not enabled, pass the frame to the IP forwarding
       * unit
       */
      if (is_myaddr(dev_net (dev),ip_hdr(sk)->daddr)) 
        {
          /*
           * if the unlabelled packet is for one of our interfaces
           * pass it to the IP receive module in our node
           */
          if (ip_hdr(sk)->version == IPV4)
            {
              /*
               * if the ip receive function is called it should be called
               * here, we do not call the IP receive function because 
               * anyway the IP handler would have processed this packet
               */
              FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
              if (ifp)
                IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
              else
                NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS, 1);
              retval = ip_rcv (sk, dev, pt, orig_dev);
              MPLS_ASSERT(retval == 0);
              /*
               * we return FAILURE from here to tell the forwarder
               * that it need not do any processing with the
               * packet whatsoever
               */
              return (FAILURE);
            }
          else if (ip_hdr(sk)->version == IPV6)
            {
#if defined (MPLS_IPV6) && defined (CONFIG_IPV6)
              FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
              if (ifp)
                IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
              else
                NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS, 1);
              retval = ipv6_rcv(sk, dev, pt);
#else
              FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_DROPPED_PKTS,
                                    1);
              if (ifp)
                IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_PKTS, 1);
              else
                NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_DROPPED_PKTS, 1);
              kfree_skb(sk);    
#endif /* MPLS_IPV6 && CONFIG_IPV6 */
              return (FAILURE);
            }
          else
            {
              FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_DROPPED_PKTS,
                                    1);
              if (ifp)
                IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_PKTS, 1);
              else
                NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_DROPPED_PKTS, 1);
              kfree_skb(sk);
              PRINTK_ERR ("l2_process : Panic !! IP packet received "
                          "with an invalid version number\n");
              return FAILURE;
            }
        }                                /* is_myaddr */
      break;

    case ETH_P_MPLS_UC:                /* labelled packet */
      shimhp = (struct shimhdr *)((uint8 *) sk->mac_header + ETH_HLEN);
      /*
       * If MPLS module is not enabled, labelled packets are 
       * nothing but garbage
       */
      PRINTK_DEBUG ("l2_process : Received an MPLS unicast packet.\n");

      /* Make sure that the incoming device is MPLS enabled */
      if (! ifp)
        {
          FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_DROPPED_LBL_PKTS,
                                1);
          NULL_IF_ENTRY_INCR_STAT (fib_handle, 
                                   IF_ENTRY_STAT_DROPPED_LABELED_PKTS, 1);
          PRINTK_DEBUG ("l2_process : MPLS not enabled on interface %s\n",
                        dev->name);
          kfree_skb(sk);
          return (FAILURE);
        }

      /*
       * Look at the label to see whether its any of the reserved
       * labels as specified in Section 2.1 of RFC 3032
       */
      label = get_label_net(shimhp->shim);

      switch (label)
        {
        case IPV4EXPLICITNULL:
	  PRINTK_DEBUG ("IPv4 Explicit Null label received\n");
          /*
           * Quoted from section 2.1 RFC 3032:
           *
           * "A value of 0 represents the "IPv4 Explicit NULL Label".
           * This label value is only legal at the bottom of the 
           * label stack. It indicates that the label stack must be 
           * popped, and the forwarding of the packet must then be 
           * based on the IPv4 header"
           */
          /*
           * a label value of IPV4EXPLICITNULL can only occur at 
           * the bottom of the label stack
           */
          MPLS_ASSERT((uint8 *) sk->network_header - (uint8 *) shimhp ==
                      SHIM_HLEN);

          if (get_bos_net(shimhp->shim))
            {
              POPLABEL(sk);

              if (LOOPBACK (ntohl(ip_hdr(sk)->daddr)) &&
                  (ip_hdr(sk)->protocol == IPPROTO_UDP))
                { 
                  /* Above check is to handle case where TTL expires but packet 
                   * had non Loopback Destination address */
                  struct udphdr *udph = NULL;
                  sk->data = (void *)ip_hdr(sk);
                  sk->len = ntohs (ip_hdr(sk)->tot_len);
                  udph = (struct udphdr*)(sk->data+(ip_hdr(sk)->ihl << 2));

                  if ((ntohs (udph->dest) == MPLS_OAM_UDP_PORT)||
                      (ntohs (udph->dest) == MPLS_BFD_UDP_PORT))
                    {
                      mpls_oam_packet_process (sk, ifp, 0, 0, 0, NULL); 
                      return (FAILURE);
                    }
                }
	      
              /*
               * call the IPV4 receive function, the unlabelled packet
               * should be processed by the IP layer.
               */
              FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
              IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
              retval = ip_rcv (sk, dev, pt, orig_dev);
              MPLS_ASSERT(retval == 0);
              return (FAILURE);
            }
          else
            {
              FIB_HANDLE_INCR_STAT (fib_handle, 
                                    FIB_HANDLE_STAT_DROPPED_LBL_PKTS, 1);
              IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_LABELED_PKTS, 1);
              PRINTK_WARNING ("l2_process : Panic !! "
                              "IPv4 Explicit Null label is not the "
                              "bottom-most label in the label stack\n");
              kfree_skb(sk);
              retval = -ENXIO;
              return (retval);
            }
          break;
        case ROUTERALERT:
          PRINTK_DEBUG ("Router Alert label received\n");
          /*
           * Quoted from section 2.1 RFC 3032:
           * A value of 1 represents the "Router Alert Label".  
           * This label value is legal anywhere in the label stack 
           * except at the bottom.  When a received packet contains
           * this label value at the top of the label stack, it is 
           * delivered to a local software module for processing. 
           * The actual forwarding of the packet is determined by 
           * the label beneath it in the stack.  However, if the 
           * packet is forwarded further, the Router Alert Label 
           * should be pushed back onto the label stack before 
           * forwarding
           */
          MPLS_ASSERT(!get_bos_net(shimhp->shim));
          if (!get_bos_net(shimhp->shim))
            {
              /*
               * store the label in the pkt for pushing it back 
               * again if the packet need to be forwarded.
               */
              router_alert = shimhp->shim;
              POPLABEL(sk);
            }
          else
            {
              PRINTK_WARNING ("l2_process : Panic !! "
                              "Router Alert label is present at the "
                              "bottom of the label stack\n");
              FIB_HANDLE_INCR_STAT (fib_handle,
                                    FIB_HANDLE_STAT_DROPPED_LBL_PKTS, 1);
              IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_LABELED_PKTS, 1);
              kfree_skb(sk);
              retval = -ENXIO;
              return (retval);
            }
          break;
        case IPV6EXPLICITNULL:
          PRINTK_DEBUG ("IPv6 Explicit Null label received\n");
          /*
           * same as IPV4EXPLICITNULL but should be handled by the
           * IPV6 module.
           */
          if (get_bos_net(shimhp->shim))
            {
              POPLABEL(sk);
              /*
               * call the IPV6 receive function, the unlabelled packet
               * should be processed by the IP layer.
               */
#if defined (MPLS_IPV6) && defined (CONFIG_IPV6)
              FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
              IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
              retval = ipv6_rcv(sk, dev, pt);
              MPLS_ASSERT(retval == 0);
#else
              FIB_HANDLE_INCR_STAT (fib_handle,
                                    FIB_HANDLE_STAT_DROPPED_LBL_PKTS, 1);
              IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_LABELED_PKTS, 1);
              kfree_skb(sk);
#endif /* MPLS_IPV6 && CONFIG_IPV6 */
              return (FAILURE);
            }
          else
            {
              PRINTK_WARNING ("l2_process : Panic !! "
                              "IPv6 Explicit Null label is not the "
                              "bottommost label in the label stack\n");
              FIB_HANDLE_INCR_STAT (fib_handle,
                                    FIB_HANDLE_STAT_DROPPED_LBL_PKTS, 1);
              IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_LABELED_PKTS, 1);
              kfree_skb(sk);
              retval = -ENXIO;
              return (retval);
            }
          break;
        case IMPLICITNULL:
          /*
           * Implicit Null label, cannot appear as a label on a labelled 
           * packet
           */
          PRINTK_ERR ("l2_process : Panic !! "
                      "Implicit NULL Label appeared in the "
                      "label encapsulation.\n");
          FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_DROPPED_LBL_PKTS,
                                1);
          IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_LABELED_PKTS, 1);
          kfree_skb(sk);
          retval = -ENXIO;
          return (retval);
          break;
	case MPLS_OAM_ITUT_ALERT_LABEL:
  
	    PRINTK_DEBUG ("OAM ALERT label received\n");
	    /*
	     * "A value of 14 represents the "MPLS OAM ALERT Label".
	     * This label value is only legal at the bottom of the 
	     * label stack. It indicates that the label stack must be 
	     * popped "
	     */
	    /*
	     * a label value of MPLS_OAM_ITUT_ALERT_LABEL can only occur at 
	     * the bottom of the label stack
	     */
	    if (get_bos_net(shimhp->shim))
	      {
		uint8 pkt_type;
		int i = 0;
		uint16 label_stack_depth = 0;
		uint32 label_stack[MAX_LABEL_STACK_DEPTH];
		POPLABEL(sk);
		memcpy(&pkt_type ,sk->data + 5, 1);
		if (pkt_type == MPLS_OAM_FDI_PROCESS_MESSAGE)
		  {
		    label_stack_depth = ntohs (*((uint16 *)(sk->data + 7)));
		    if ( label_stack_depth < MAX_LABEL_STACK_DEPTH )
		      {
			for (i = 0; i < label_stack_depth; i++)
			  {
			    memcpy (&label_stack[i], sk->data + 11 + (i*4), 4);
			    PUSHLABEL(sk, label_stack[i], 0);
			  }
			/*needs to set label stack depth zero in data,to
			  avoid looping of FDI packets */
			label_stack_depth = 0;
			memcpy(sk->data+7, &label_stack_depth, 2);
		      }
		  }
		return (SUCCESS);
	      }
	    else
	      {
		FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_DROPPED_LBL_PKTS,
				      1);
		IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_LABELED_PKTS, 1);
		PRINTK_WARNING ("l2_process : Panic !! "
				"MPLS OAM ALERT  label is not the "
				"bottom-most label in the label stack\n");
		kfree_skb(sk);
		retval = -ENXIO;
		return (retval);
	      }
	    break;	  
        }
      
      /*
       * Values 4-15 are reserved, they cannot be used as ordinary 
       * label values
       */
      if (RESERVEDLABEL(label))
        {
          kfree_skb(sk);
          retval = -ENXIO;
          FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_DROPPED_LBL_PKTS,
                                1);
          IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_LABELED_PKTS, 1);
          PRINTK_ERR ("l2_process :  Panic !! "
                      "Reserved label [%lu] found on the label "
                      "stack\n", label);
          MPLS_ASSERT(0);
          return (retval);
        }

      break;
    default:
      FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
      if (ifp)
        IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
      else
        NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS, 1);
      PRINTK_ERR ("l2_process :  Panic !!  Received a packet whose Ethernet "
                  "protocol field matches neither IP, MPLS or ARP... "
                  "Passing to IP.\n");

      retval = ip_rcv (sk, dev, pt, orig_dev);

      return (FAILURE);
    }
  return SUCCESS;
}

/*
 * the forwarder. The sk_buff passed should be freed when this function
 * returns.
 */
void
fwd_forward_frame (struct sk_buff *sk, struct net_device *dev,
                   struct packet_type *pt, struct mpls_interface *ifp,
                   struct net_device *orig_dev)
{
  /*---------------------------------------------------------------------------
    We copy the protocol pointers from the rcvd_frame to local pointers.
    This saves us a lot of keystrokes and ugly long lines.
    -------------------------------------------------------------------------*/

  struct ethhdr      *ethhp = (struct ethhdr *)sk->mac_header;
  struct iphdr       *iphp = ip_hdr(sk);
  struct shimhdr     *shimhp = NULL;
  struct shimhdr     *shp = NULL;
  int                in_label;
  struct ftn_entry   *ftn_entry = NULL;
  struct ilm_entry   *ilm_entry;
  struct mpls_table  *table;
  int                vrf_search;
  int                second_time;
  int                ret;
  uint32             d_addr;
  uint32             ftn_ix = INVALID_FTN_IX;

  /*
   * for labelled packet there is a shim between layer2 and layer3 headers
   */
  if ((uint8 *) iphp - (uint8 *) ethhp > ETH_HLEN)
    {
      shimhp = (struct shimhdr *)((uint8 *) ethhp + ETH_HLEN);
      /*
       * from now on the condition for labelled packet is (shimhp != NULL)
       */
    }
  if (! shimhp)
    {
      second_time = 0;
      vrf_search = 0;
      
      /* Get destination */
      d_addr = iphp->daddr;

    lookup_ftn_again:

      if (ftn_entry && ftn_entry->bypass_ftn_ix && second_time > 0)
	{
	  ftn_ix = ftn_entry->bypass_ftn_ix;
	}

      /* Figure out which FTN/VRF table use */
      if (ifp && ifp->vrf && (second_time != 1))
        {
          vrf_search = 1;
          table = ifp->vrf;
        }
      else
        {
          table = fib_handle->ftn;
        }

      /* Find the best FTN entry */
      if (vrf_search)
        ftn_entry = fib_best_ftnentry (table, sk, d_addr, RT_LOOKUP_INPUT, 1);
      else
        ftn_entry = fib_best_ftnentry (table, sk, d_addr, RT_LOOKUP_INPUT, 0);
      if (ftn_entry != NULL)
        {
          /* FTN statistics */
          FTN_ENTRY_INCR_STAT (ftn_entry, FTN_ENTRY_STAT_MATCHED_PKTS, 1);
          /* sk->len : (rx case) excluded L2 header from the length */
          FTN_ENTRY_INCR_STAT (ftn_entry, FTN_ENTRY_STAT_MATCHED_BYTES, sk->len);

          /* If a match is found in the FTN table the forwarder
             executes the opcode list associated with the NHLFE
             corresponding to the FTN entry.  */
          ret = forwarder_process_nhlfe(&sk, dev, pt, ftn_entry->nhlfe_list,
                                        ifp, 0, NULL);
          if (ret == DO_SECOND_FTN_LOOKUP)
            {
              /* Set second-lookup flag */
              second_time = 1;
              
              /* Set second lookup ftn key */
              d_addr = ftn_entry->nhlfe_list->rt->rt_gateway;
              if (d_addr == 0)
                goto ftn_drop;

              /* Only go back if there is a valid ftn_key */
              goto lookup_ftn_again;
            }
          else if (ret == FAILURE)
            {
              PRINTK_ERR ("fwd_forward_frame : "
                          "Panic !! NHLFE could not be processed, "
                          "FTN matched unlabelled packet "
                          "releasing sk_buff\n");
              goto ftn_drop;
            }
          else if (ret == DO_IP_RCV)
            {
              PRINTK_DEBUG ("fwd_forward_frame : "
                            "No NHLFE entry found for FTN "
                            "[%d.%d.%d.%d] that was received from "
                            "[%d.%d.%d.%d] on interface %s. "
                            "Sending to IP.\n",
                            NIPQUAD(d_addr), NIPQUAD (iphp->saddr),
                            dev->name);
              FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
              if (ifp)
                IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
              else
                NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS, 1);
              ret = ip_rcv (sk, dev, pt, orig_dev);
              MPLS_ASSERT (ret == 0);
              return;
            }
          else if (ret == (-ENOMEM))
            if (ftn_entry->nhlfe_list)
              NHLFE_ENTRY_INCR_STAT (ftn_entry->nhlfe_list, 
                                     NHLFE_ENTRY_STAT_DISCARD_PKTS, 1);

          FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_LABELED_TX_PKTS,1);
          if (ifp)
            IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_LABELED_TX_PKTS, 1);
          else
            NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_LABELED_TX_PKTS,
                                     1);

          /* FTN statistics */
          FTN_ENTRY_INCR_STAT (ftn_entry, FTN_ENTRY_STAT_PUSHED_PKTS, 1);
          FTN_ENTRY_INCR_STAT (ftn_entry, FTN_ENTRY_STAT_TX_BYTES, 
                               (sk->len - ETH_HLEN));
          if (ftn_entry->nhlfe_list)
            {
              /* Out-Segment statistics */
              NHLFE_ENTRY_INCR_STAT (ftn_entry->nhlfe_list, 
                                     NHLFE_ENTRY_STAT_TX_PKTS, 1);
              /* sk->len : (tx case) included L2 header in the length,
                 it sholud exclude a L2 header length from a packet length */
              NHLFE_ENTRY_INCR_STAT (ftn_entry->nhlfe_list, 
                                     NHLFE_ENTRY_STAT_TX_BYTES,
                                     (sk->len - ETH_HLEN));
            }
        }
      else
        {
          /* Reset shimhp */
          shimhp = NULL;

          /* Check if any labels have been added to the packet */
          if ((((uint8 *)ip_hdr(sk)) - ((uint8 *)(sk->mac_header)))
              > ETH_HLEN)
            {
              /* There is atleast one label */
              shimhp = (struct shimhdr *)((uint8 *)(sk->mac_header)
                                          + ETH_HLEN);
            }

          /* If this was a vrf lookup or there was a shim, drop the packet */
          if (vrf_search || shimhp)
            {
              PRINTK_DEBUG ("fwd_forward_frame : "
                            "Post VRF-search operation failed OR label found "
                            "before sending to IP. Dropping packet.\n");
              goto ftn_drop;
            }

          /*
           * Now pass the unlabelled packet to our IP module for normal IP 
           * forwarding
           */
          if (ip_hdr(sk)->version == IPV4)
            {
              PRINTK_DEBUG ("fwd_forward_frame : "
                            "No FTN entry found for destination "
                            "[%d.%d.%d.%d] that was received from "
                            "[%d.%d.%d.%d] on interface %s. "
                            "Sending to IP.\n",
                            NIPQUAD(d_addr), NIPQUAD (ip_hdr(sk)->saddr),
                            dev->name);
              FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
              if (ifp)
                IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
              else
                NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS,
                                         1);
              ret = ip_rcv (sk, dev, pt, orig_dev);
            }
#if defined (MPLS_IPV6) && defined (CONFIG_IPV6)
          else if (ip_hdr(sk)->version == IPV6)
            {
              FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
              if (ifp)
                IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
              else
                NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS,
                                         1);
              ret = ipv6_rcv(sk, dev, pt);
            }
#endif /* MPLS_IPV6 && CONFIG_IPV6 */

          MPLS_ASSERT(ret == 0);
        }                                /* FTN Entry Not found */
    }                                    /* unlabelled packet */
  else                                /* labelled packet */
    {
      uint32 label_stack_depth = 0;
      uint32 label_stack[MAX_LABEL_STACK_DEPTH];
      int i;

      /* Check if incoming interface is MPLS enabled */
      if ((ifp == NULL) || (ifp->ilm == NULL) ||
          (ifp->status == MPLS_DISABLED))
        {
          FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_DROPPED_LBL_PKTS,
                                1);
          if (ifp)
            IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_LABELED_PKTS, 1);
          else
            NULL_IF_ENTRY_INCR_STAT (fib_handle, 
                                     IF_ENTRY_STAT_DROPPED_LABELED_PKTS, 1);
          PRINTK_ERR ("Error : Received a labeled packet on "
                      "interface %s, which is not MPLS enabled\n",
                      dev->name);
          kfree_skb(sk);
          return;
        }

      label_stack_depth = ((uint8 *) iphp - (uint8 *) ethhp - ETH_HLEN)/4; 
      for (i = 0; i < label_stack_depth; i++)
        { 
          shp = (struct shimhdr *)((uint8 *) ethhp + ETH_HLEN + (i * 4));
          label_stack[i] = get_label_net (shp->shim);
        }
 
      in_label = get_label_net(shimhp->shim);
      
      /*labelled IP packet. Lookup ILM */
      ilm_entry = fib_find_ilmentry(ifp->ilm, 0 /*ignored */, in_label,
                                    dev->ifindex, FIB_FIND, &ret);

      if ((ilm_entry != NULL) && (ret == SUCCESS))
        {
          /* In-Segment statistics */
          ILM_ENTRY_INCR_STAT (ilm_entry, ILM_ENTRY_STAT_RX_PKTS, 1);
          /* sk->len : (rx case) excluded L2 header from the length */
          ILM_ENTRY_INCR_STAT (ilm_entry, ILM_ENTRY_STAT_RX_BYTES, sk->len);
          ILM_ENTRY_INCR_STAT (ilm_entry, ILM_ENTRY_STAT_RX_HC_BYTES, sk->len);

          ret = forwarder_process_nhlfe(&sk, dev, pt, ilm_entry->nhlfe_list,
                                        ifp, label_stack_depth, label_stack);

	  switch (ret)
	    {
	    case DO_SECOND_FTN_LOOKUP :
	      {
		/* Set second lookup ftn key */
		d_addr = ilm_entry->nhlfe_list->rt->rt_gateway;
		if (d_addr == 0)  
		  goto ilm_drop;

		/* lookup ftn */
		ftn_entry = fib_best_ftnentry (fib_handle->ftn, sk, d_addr,
                                               RT_LOOKUP_INPUT, 1);
		if (ftn_entry == NULL)
		  goto ilm_drop;

		/* FTN statistics */
		FTN_ENTRY_INCR_STAT (ftn_entry, FTN_ENTRY_STAT_MATCHED_PKTS, 1);
		/* sk->len : (rx case) excluded L2 header from the length */
		FTN_ENTRY_INCR_STAT (ftn_entry, FTN_ENTRY_STAT_MATCHED_BYTES,
                                     sk->len);
  
		ret = forwarder_process_nhlfe (&sk, dev, pt, 
                                               ftn_entry->nhlfe_list, ifp,
                                               label_stack_depth, label_stack);

		if (ret != SUCCESS)
		  {
		    goto ftn_drop;
		  }

		/* Statistics */              
	      }
	      break;
	    case FAILURE :
	      {
		PRINTK_ERR ("fwd_forward_frame : "
			    "Panic !! NHLFE could not be processed "
			    "releasing frame\n");
		goto ilm_drop;
	      }
	      break;
	    case DO_IP_RCV :
	      {
		PRINTK_DEBUG ("fwd_forward_frame : "
			      "No NHLFE entry found for FTN "
			      "[%d.%d.%d.%d] that was received from "
			      "[%d.%d.%d.%d] on interface %s. "
			      "Sending to IP.\n",
			      NIPQUAD(d_addr), NIPQUAD (ip_hdr(sk)->saddr),
			      dev->name);
		FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_IP_TX_PKTS, 1);
              if (ifp)
                IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_IP_TX_PKTS, 1);
              else
                NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_IP_TX_PKTS,
                                         1);
              ret = ip_rcv (sk, dev, pt, orig_dev);
              MPLS_ASSERT (ret == 0);
              return;
	      }
	      break;
	    case (-ENOMEM) :
	      NHLFE_ENTRY_INCR_STAT (ilm_entry->nhlfe_list, 
				     NHLFE_ENTRY_STAT_DISCARD_PKTS, 1);
	      break;
	    case MPLS_RET_DST_LOOPBACK : 
	    case MPLS_RET_TTL_EXPIRED :
	      {
		if (LOOPBACK (ntohl(ip_hdr(sk)->daddr)) &&
		    (ip_hdr(sk)->protocol == IPPROTO_UDP))
  		  { 
                   /* Above check is to handle case where TTL expires but packet                    * had non Loopback Destination address */
 		    struct udphdr *udph = NULL;
                    sk->data = (void *)ip_hdr(sk);
                    sk->len = ntohs (ip_hdr(sk)->tot_len);
                    udph = (struct udphdr*)(sk->data+(ip_hdr(sk)->ihl << 2));
		    
		    if ((ntohs (udph->dest) == MPLS_OAM_UDP_PORT) ||
                         (ntohs (udph->dest) == MPLS_BFD_UDP_PORT))
		      {
                        mpls_oam_packet_process (sk, ifp, 0, 0,
                                                 label_stack_depth, 
                                                 label_stack); 
                      }
		  }
	      }
	      break;
	    default :
	      break;
	    }                

	  FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_SWITCHED_PKTS, 1);
	  IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_SWITCHED_PKTS, 1);
	  
          /* In-Segment statistics */
          ILM_ENTRY_INCR_STAT (ilm_entry, ILM_ENTRY_STAT_TX_BYTES, 
                               (sk->len - ETH_HLEN));
          if (get_bos_net (shimhp->shim))
            ILM_ENTRY_INCR_STAT (ilm_entry, ILM_ENTRY_STAT_POPPED_PKTS, 1);
          else
            ILM_ENTRY_INCR_STAT (ilm_entry, ILM_ENTRY_STAT_SWAPPED_PKTS, 1);

          /* Out-Segment statistics */
          NHLFE_ENTRY_INCR_STAT (ilm_entry->nhlfe_list, 
                                 NHLFE_ENTRY_STAT_TX_PKTS, 1);
          /* sk->len : (tx case) included L2 header in the length,
             it sholud exclude a L2 header length from a packet length */
          NHLFE_ENTRY_INCR_STAT (ilm_entry->nhlfe_list, 
                                 NHLFE_ENTRY_STAT_TX_BYTES,
                                 (sk->len - ETH_HLEN));
        } /* ILM entry found */
      else
        {
          /*
           * ILM entry not found. RFC says that we should 
           * discard the packet.
           */
          PRINTK_ERR ("fwd_forward_frame : Panic !! "
                      "No ILM entry found for label %d\n",
                      (int)in_label);
          goto ilm_drop;
        }
    } /* Labeled packet */

  return;

 ftn_drop:
  FIB_HANDLE_INCR_STAT (fib_handle, FIB_HANDLE_STAT_DROPPED_PKTS, 1);
  if (ifp)
    IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_PKTS, 1);
  else
    NULL_IF_ENTRY_INCR_STAT (fib_handle, IF_ENTRY_STAT_DROPPED_PKTS, 1);

  if (ftn_entry && ftn_entry->nhlfe_list)
    NHLFE_ENTRY_INCR_STAT (ftn_entry->nhlfe_list, 
                           NHLFE_ENTRY_STAT_ERROR_PKTS, 1);
  kfree_skb(sk);
  return;

 ilm_drop:
  FIB_HANDLE_INCR_STAT (fib_handle,
                        FIB_HANDLE_STAT_DROPPED_LBL_PKTS, 1);
  /* mplsInterfacePerf.LabelLookupFailures */
  IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_DROPPED_LABELED_PKTS, 1);
  if (ilm_entry && ilm_entry->nhlfe_list)
    NHLFE_ENTRY_INCR_STAT (ilm_entry->nhlfe_list, 
                           NHLFE_ENTRY_STAT_ERROR_PKTS, 1);
  kfree_skb(sk);
  return;
}


/*
 * sk_buff should have the 'dst' field set properly. sk_buff should be freed 
 * when this function returns.
 */
int
mpls_forward (struct sk_buff *sk)
{
  /*
   * whether the sk_buff contains labelled packet
   */
  int labelled = ! (sk->data == sk->network_header);
    
  /* Maximum allowable size of each outgoing fragment.  */
  uint32 frag_size;
  uint32 next_hop_mtu;
  struct mpls_interface *ifp;

  if (! sk->dst)
    {
      kfree_skb(sk);
      PRINTK_ERR ("mpls_forward : Panic !! sk->dst "
                  "not set\n");
      return -1;
    }

  if (! sk->dst->dev)
    {
      kfree_skb(sk);
      PRINTK_ERR ("mpls_forward : Panic !! "
                  "No device associated with dst\n");
      return -1;
    }

  sk->dev = sk->dst->dev;

  if (! (sk->dev->flags & IFF_UP))
    PRINTK_ERR ("mpls_forward : Outgoing device [%s] is DOWN\n",
                sk->dev->name);

  if (! sk->dst->ops)
    {
      kfree_skb(sk);
      PRINTK_ERR ("mpls_forward : Panic !! "
                  "No ops\n");
      return -1;
    }

  sk->protocol = sk->dst->ops->protocol;
    
  /*
   * hack for Implicit Null case
   */ 
  if(! labelled)
    {
      ((struct ethhdr *)(sk->mac_header))->h_proto = __constant_htons(ETH_P_IP);
    }

  /*
   * fragment size cannot be more than the outgoing device  mtu 
   */
  frag_size = sk->dst->dev->mtu;
  /*
   * fragment size is the minimum of the maximum fragment size as 
   * imposed by the MILDS and the maximum fragment size as imposed
   * by the outgoing device mtu
   */
  if (max_frag_size)
    frag_size = MIN(sk->dst->dev->mtu, max_frag_size);
  /*
   * prepare for the next packet
   */
  max_frag_size = 0;

  if (sk->len <= frag_size)
    {
      PRINTK_DEBUG ("mpls_forward : Sending non-fragmented packet\n");
      /*
       * We have to do this because of the following line in 
       * neigh_resolve_output
       * __skb_pull(skb, skb->nh.raw - skb->data);
       * It expects that the layer3 header starts where sk->data points
       */
      sk->network_header = sk->data;
      l2_send_frame(sk);
    }
  else
    {
      if ((ntohs(ip_hdr(sk)->frag_off) & IP_DF))
        {
          /*
           * Fragmentation needed but DF bit set
           * Next-Hop MTU should be set to the difference between
           * the Effective Maximum Frame Payload Size and the
           * total size in bytes of all the label stack entries.
           * next_hop_mtu = ETHMTU - ((uint8*)sk->frame_iphp - 
           *                          (uint8*)shimhp);
           * The ICMP error to be sent is 'Fragmentation needed
           * but DF bit set'.
           */
          next_hop_mtu = frag_size - ((int)sk->network_header - (int)sk->data);
          PRINTK_DEBUG ("mpls_forward : Sending ICMP "
                        "'Fragmentation needed but DF bit set'\n");

          kfree_skb (sk);
          return FAILURE;
        }

      ifp = mpls_if_lookup ((uint32)sk->dst->dev->ifindex);
      if (ifp)
        IF_ENTRY_INCR_STAT (ifp, IF_ENTRY_STAT_OUT_FRAGMENTS, 1);
      PRINTK_DEBUG ("mpls_forward : Sending fragmented packet of "
                    "max size [%d]\n", (int)frag_size);
      return mpls_fragment(sk, frag_size);
    }

  return SUCCESS;
}


/*===========================================================================
 *                          INTERNAL FUNCTIONS
 ==========================================================================*/

/*---------------------------------------------------------------------------
 * The follwing function procesess a list of NHLFE entries.
 *--------------------------------------------------------------------------*/
sint8
forwarder_process_nhlfe(struct sk_buff ** sk, struct net_device *dev,
                        struct packet_type *pt, struct nhlfe_entry *list,
                        struct mpls_interface *ifp, uint32 label_stack_depth,
                        uint32 *label_stack)
{
  struct label_primitive  *label_primitive;
  uint32            tos_label = 0;    /* top-of-stack label */
  sint32            last_stack_opcode = INVALID;    /*last opcode executed */
  uint32            label_to_push;
  struct nhlfe_entry      *nhlfe;
  LABEL             newlabel;
  struct shimhdr          *shimhp = NULL;
  L3ADDR            nhop = 0;                /* nexthop from ILM/FTN table */
  uint32            daddr;
  struct iphdr     *iph;
  int               last_label = 0;
  int               ret;
  struct sk_buff   *pskb;
  int input1 = 0;
  int input2 = 0;
  uint16_t ach_type = 0;

  if (!sk || (*sk == NULL) || !list)
    {
      PRINTK_ERR ("forwarder_process_nhlfe : Invalid parameters\n");
      return FAILURE;
    }
  
  /*
   * If its a labelled packet, then shim header starts from skb->data
   */
  if ((*sk)->network_header != (*sk)->data)
    {
      shimhp = (struct shimhdr *) (*sk)->data;

      if (!shimhp)
        return FAILURE;

      if (get_bos_net (shimhp->shim))
        {
          /* This is the last label */
          last_label = 1;
        }
    }
  
  /* Find the best nhlfe entry */
  nhlfe = forwarder_find_best_nhlfe(list);
  MPLS_ASSERT(nhlfe);
  if (!nhlfe)
    return FAILURE;

  label_primitive = nhlfe->primitive;
  if (!label_primitive)
      return FAILURE;
  pskb = skb_unshare (*sk, GFP_ATOMIC);
  if (pskb == NULL)
    return -ENOMEM;

  /* To prevent from a crashed machine (some machine happened),
     skb->ip_summed set to CHECKSUM_NONE in dst_neigh_output(). */
  *sk = pskb;

  for (; label_primitive; label_primitive = label_primitive->next)
    {
      switch (label_primitive->opcode)
        {
        case POP:
          if (!shimhp)
            return FAILURE;

          last_stack_opcode = POP;
          tos_label = get_label_net(shimhp->shim);

          /* Before popping label, check the TTL Expiry case */
          ret = check_and_update_ttl (*sk, shimhp, MPLS_FALSE);

          if (ret <= 0)
            {
              /* Pop the label before returning failure */
              POPLABEL(*sk);
              /* Report failure */
              return (MPLS_RET_TTL_EXPIRED);
            }

          PRINTK_DEBUG ("Popping label ...%d\n", (int)tos_label);

          POPLABEL(*sk);
          
          /* reset shimhp if all labels have been popped */
          if (last_label)
	    {
	      if (LOOPBACK (ntohl(ip_hdr(*sk)->daddr)))
		{
		  return MPLS_RET_DST_LOOPBACK;
		}
	      shimhp = NULL;
	    }
          else
            shimhp = (struct shimhdr *) (*sk)->data;

          break;

        case POP_FOR_VPN:
          if (!shimhp)
            return FAILURE;
	  
          ret = check_and_update_ttl (*sk, shimhp, MPLS_TRUE);
          if (ret <= 0)
            {
              /* Report failure */
              return (MPLS_RET_TTL_EXPIRED);
            }

          last_stack_opcode = POP_FOR_VPN;
          tos_label = get_label_net(shimhp->shim);

          PRINTK_DEBUG ("Pop label for VPN ... %d\n", (int)tos_label);

          POPLABEL(*sk);

	  if (LOOPBACK (ntohl(ip_hdr(*sk)->daddr)))
	    {
	      return MPLS_RET_DST_LOOPBACK;
	    }

          break;

        case DLVR_TO_IP:

          ret = check_and_update_ttl (*sk, shimhp, MPLS_TRUE);
          if (ret <= 0)
            {
              /* Report failure */
              return (FAILURE);
            }

          PRINTK_DEBUG ("Delivering to IP for VPN ... \n");

          last_stack_opcode = DLVR_TO_IP;
          break;

        case PUSH:

          ret = check_and_update_ttl (*sk, shimhp, MPLS_TRUE);
          if (ret <= 0)
            {
              /* Report failure */
              return (FAILURE);
            }

          last_stack_opcode = PUSH;
          label_to_push = label_primitive->label;

          PRINTK_DEBUG ("Pushing label ...%d\n", (int)label_to_push);

          PUSHLABEL(*sk, label_to_push, 0); 
          break;

        case PUSH_AND_LOOKUP:

          ret = check_and_update_ttl (*sk, shimhp, MPLS_FALSE);
          if (ret <= 0)
            {
              /* Report failure */
              return (FAILURE);
            }

          last_stack_opcode = PUSH_AND_LOOKUP;
          label_to_push = label_primitive->label;

          PRINTK_DEBUG ("Pushing label for indirectly connected peer ...%d\n",
                        (int)label_to_push);

          PUSHLABEL(*sk, label_to_push, 0); 
          break;

        case SWAP:
          if (!shimhp)
            return FAILURE;

          ret = check_and_update_ttl (*sk, shimhp, MPLS_TRUE);
          if (ret <= 0)
            {
              /* Report failure */
              return (MPLS_RET_TTL_EXPIRED);
            }

          newlabel = label_primitive->label;
          tos_label = get_label_net(shimhp->shim);

          PRINTK_DEBUG ("Swapping label %d with %d...\n",
                        (int)tos_label, (int)newlabel);
          /*
           * If the label to be swapped is the Implicit Null label, then
           * instead of swapping the old top-of-stack label with the 
           * implicit null label, the LSR is supposed to pop the old 
           * top-of-stack label and forward the packet as a plain IP
           * packet
           * Refer 4.1.5 RFC 3031
           */
          if (newlabel == IMPLICITNULL)
            {
              PRINTK_DEBUG ("Label to be swapped is Implicit Null label\n");

              last_stack_opcode = POP;
              POPLABEL(*sk);

	      if (last_label && LOOPBACK (ntohl(ip_hdr(*sk)->daddr)))
		{
		  return MPLS_RET_DST_LOOPBACK;
		}
            }
          else
            {
              last_stack_opcode = SWAP;
              SWAPLABEL(*sk, newlabel);
            }
          break;

        case SWAP_AND_LOOKUP:
          if (!shimhp)
            return FAILURE;

          ret = check_and_update_ttl (*sk, shimhp, MPLS_FALSE);
          if (ret <= 0)
            {
              return (MPLS_RET_TTL_EXPIRED);
            }

          newlabel = label_primitive->label;
          if (newlabel == IMPLICITNULL)
            return (FAILURE);

          tos_label = get_label_net (shimhp->shim);

          PRINTK_DEBUG ("Swapping label %d with %d...\n",
                        (int)tos_label, (int)newlabel);

          last_stack_opcode = SWAP_AND_LOOKUP;
          SWAPLABEL(*sk, newlabel);
          break;

        case FTN_LOOKUP: 

          /* Added for BGP routes. */
          ret = check_and_update_ttl (*sk, shimhp, MPLS_FALSE);
          if (ret <= 0)
            {
              /* Report failure */
              return (FAILURE);
            }

          last_stack_opcode = FTN_LOOKUP;
          break;

        case POP_FOR_VC:
          if (!shimhp)
            return FAILURE;

          if (last_label)
            {
              /* if first nibble is 0001b, set cc type as type 1 */
              if (IS_PW_ACH(*((uint8 *)(shimhp) + SHIM_HLEN)))
                {
                  /* label_stack_depth calculation in the caller would
                   * have consider PWACH also as label, 
                   * update the label stack depth correctly */
                  if (label_stack_depth > 0)
                  label_stack[--label_stack_depth] = 0;

                  /* Move the shimhp to SHIMHDR len, and get the channel type */
                  ach_type = 
                      ntohs (get_channel_type_net(*(((uint32 *)(shimhp) + 1))));
                  /* If Channel type in PW ACH is BFD, set the input2 to
                   * contain the BFD packet encapsulation used as RAW */
                  if (ach_type == MPLS_PW_ACH_BFD)
                   input2 = BFD_CV_RAW_ENCAP;
  
                  SET_FLAG (input1, CC_TYPE_1_BIT);
                }
  
              /* if router alert label is set, set cc type as type2 */
              if (router_alert)
                SET_FLAG (input1, CC_TYPE_2_BIT);

              /* New block  to have the local shimhp */
              {
                /* Local shimhp */
                struct shimhdr *shimhp = NULL;
                shimhp = 
                  (struct shimhdr *)((uint8 *)(*sk)->mac_header + ETH_HLEN);

                /* if TTL = 1, set cc type as type 3 */
                if (get_ttl_net(shimhp->shim) == 1)
                  {
                    SET_FLAG (input1, CC_TYPE_3_BIT);
                  }

                /* Pop, fix pointers, and send directly to driver. */
                tos_label = get_label_net(shimhp->shim);
                PRINTK_DEBUG ("Popping label for Virtual Circuit...%d\n",
                    (int)tos_label);
              }

              /* If VCCV packet (input1 != 0) 
               * or VC OAM packet, send it for control plane processing */
              if ((input1) || (last_label && ip_hdr(*sk)->version == IPV4 && 
                    LOOPBACK (ntohl(ip_hdr(*sk)->daddr))
                    && ip_hdr(*sk)->protocol == IPPROTO_UDP))
                {
                  /* Pop the label before sending to the control plane */
                  POPLABEL(*sk);
                  mpls_oam_packet_process (*sk, ifp, input1,
                      input2, label_stack_depth, label_stack);
                  return SUCCESS;
                }
            }

          return mpls_pop_and_fwd_vc_label (*sk, nhlfe->rt, shimhp);

        default:

          PRINTK_ERR ("Unknown opcode %d in label_primitive\n",
                      label_primitive->opcode);
          break;
        }
    }
  /*
   * if the top label was a ROUTERALERT label then we do all the processing
   * based on the label beneath the ROUTERALERT label, but finally we have
   * to push the ROUTERALERT label back
   */

  if (router_alert)
    {
      uint32 router_alert_label = get_label_net(router_alert);

      last_stack_opcode = PUSH;
      PUSHLABEL(*sk, router_alert_label, 0); 
      /*
       * EXP and TTL fields are to be set here
       */
      router_alert = 0;
    }

  /* If the opcode was PUSH_AND_LOOKUP, we need to do a second
     lookup */
  if ((last_stack_opcode == PUSH_AND_LOOKUP) ||
      (last_stack_opcode == FTN_LOOKUP) ||
      (last_stack_opcode == SWAP_AND_LOOKUP))
    return DO_SECOND_FTN_LOOKUP;
  
  /*
   * If the next hop is the same LSR itself the packet should again be passed
   * to the forwarder, as if we just received this packet with one label 
   * removed.
   */
  if (nhlfe->rt)
    nhop = nhlfe->rt->rt_gateway;
  else
    return FAILURE;

  /* Get the ip header */
  iph =  ip_hdr(*sk);
  
  /* Get the dest ip addr */
  daddr = iph->daddr;

  /* If this packet is meant for us */
  if (nhop == 0 && is_myaddr (dev_net(dev), daddr))
    {
      if (last_label == 0)
        {
          /* There is another label in the stack */
          PRINTK_ERR ("Second label found for local packet\n");

          /* Report failure */
          return (FAILURE);
        }

      goto ip_fwding;
    }
#ifdef COMMNENTED  
  if (check_for_ttl)
    {
      ret = check_and_update_ttl (*sk, shimhp, MPLS_TRUE);
      if (ret <= 0)
        {
          /* Report failure */
          return (FAILURE);
        }
    }
#endif /* COMMNENTED */  

  /* If this is the last label.... */
  if (last_label == 1)
    {
      if (last_stack_opcode == POP)
        {
          if (nhop == 0)
            goto ip_fwding;
          else
            goto mpls_ip_fwding;
        }
      else if (last_stack_opcode == POP_FOR_VPN)
        {
          goto vpn_fwding;
        }
    }

  if (last_stack_opcode == DLVR_TO_IP)
    {
      goto vpn_fwding;
    }

  /* If here, then packet needs to be label switched */
  mpls_pre_forward (*sk, list->rt, MPLS_LABEL_FWD);
  return (SUCCESS);

 ip_fwding:
  (*sk)->dst = NULL;
  
  return DO_IP_RCV;

 mpls_ip_fwding:
  mpls_pre_forward (*sk, list->rt, MPLS_IP_FWD);

  return SUCCESS;

 vpn_fwding:
  mpls_pre_forward (*sk, list->rt, MPLS_VPN_FWD);

  return SUCCESS;
}

/*---------------------------------------------------------------------------
 * Choose the best NHLFE entry to use , from  a list.
 --------------------------------------------------------------------------*/
struct nhlfe_entry*
forwarder_find_best_nhlfe (struct nhlfe_entry * list)
{
  if (list->next)
    {
      PRINTK_DEBUG ("forwarder_find_best_nhlfe : "
                    "more than one NHLFE found \n");
      /* Here decide on which is the best one to use */
      /* Right now just return what we have */
      return (list);
    }
  else
    {
      return (list);
    }
}

/* Routine to process packets before sending them out. */
int
mpls_pre_forward (struct sk_buff *skb, struct rtable *rt, u8 fwd_type)
{
  int result;
  struct rtable *rt2 = NULL;
  struct dst_entry *dst2;

  if (skb == NULL)
    {
      PRINTK_ERR ("mpls_pre_forward: Invalid parameters\n");
      return FAILURE;
    }

  /* If rt is NULL, get out */
  if ((rt == NULL) || (rt->u.dst.dev == NULL))
    {
      kfree_skb (skb);
      PRINTK_ERR ("mpls_pre_forward: Invalid rtable entry passed  \n");
      return FAILURE;
    }
  
  /* Make a copy of the rt for use in forwarding this packet. */
  rt2 = rt_make_copy (rt);
  if ((rt2 == NULL) || (rt2->u.dst.dev == NULL))
    {
      kfree_skb (skb);
      PRINTK_ERR ("mpls_pre_forward: Unable to make copy of rt\n");
      return FAILURE;
    }
  
  dst2 = &rt2->u.dst;

  if (((struct ethhdr *)(skb->mac_header))->h_proto == __constant_htons (ETH_P_ARP))
    {
      struct arphdr *arph = (struct arphdr *)skb->network_header;
      u8 *ptr;
       
      if (!arph)
        {
          kfree_skb (skb);
          PRINTK_ERR ("mpls_pre_forward : Invalid ARP header \n");
          return FAILURE;
        }
      if ((arph->ar_pro != __constant_htons (ETH_P_IP)) ||
          (arph->ar_pln != 4))
        {
          kfree_skb (skb);
          PRINTK_ERR ("mpls_pre_forward : Invalid Protocol type in ARP "
                      "packet \n");
          return FAILURE;
        }
      ptr = (u8 *)arph;
      ptr += sizeof (struct arphdr);
      ptr += 6;
      memcpy (&rt2->rt_src, ptr, 4);
      ptr += 10;
      memcpy (&rt2->rt_dst, ptr, 4);
    }
  else 
    {
      rt2->rt_dst = ip_hdr(skb)->daddr;
      rt2->rt_src = ip_hdr(skb)->saddr;
    }
  
  /* For labeled forwarding, no incoming interface is required. */
  if ((fwd_type == MPLS_LABEL_FWD) || (fwd_type == MPLS_VC_FWD))
    rt2->rt_iif = 0;
  else
    rt2->rt_iif = skb->dev->ifindex;
  
  /* Make gateway the same as the destination, since this is a directly
     connected case. */
  if (rt2->rt_gateway == 0)
    rt2->rt_gateway = rt2->rt_dst;

  /* Fill dst input. */
  if (dst2->input == NULL)
    {
      if ((fwd_type == MPLS_VPN_FWD) || (fwd_type == MPLS_IP_FWD))
        {
#if 0
          if (mpls_dst_input == NULL)
            {
              dst_data_init (rt2->rt_gateway, dst2->dev, SET_DST_ALL, 
                             RT_LOOKUP_INPUT);
            }
#endif    
          dst2->input = mpls_forward;
        }
      else if ((fwd_type == MPLS_LABEL_FWD) || (fwd_type == MPLS_VC_FWD))
        dst2->input = mpls_forward;
    }

  /* Fill dst ops. */
  if (dst2->ops == NULL)
    {
      if ((fwd_type == MPLS_VPN_FWD) || (fwd_type == MPLS_IP_FWD))
        {
          if (dst_ops == NULL)
            {
              dst_data_init (rt2->rt_gateway, dst2->dev, SET_DST_OPS, 
                             RT_LOOKUP_INPUT);
            }
          
          dst2->ops = dst_ops;
        }
      else if ((fwd_type == MPLS_LABEL_FWD) || (fwd_type == MPLS_VC_FWD))
        dst2->ops = &labelled_dst_ops;
    }
  
  /* Both input and ops MUST be valid. */
  if ((dst2->input == NULL) || (dst2->ops == NULL))
    {
      PRINTK_ERR ("mpls_pre_forward: Invalid arguments : "
                  "dst->input = 0x%p, dst->ops = 0x%p\n",
                  dst2->input, dst2->ops);
      kfree_skb (skb);
      dst_release (dst2);
      rt_free (rt2);
      return (FAILURE);
    }

  /* For VRF case, make sure that the outgoing interface is enabled for
     VRF. */
  if (fwd_type == MPLS_VPN_FWD)
    {
      struct mpls_interface *ifp = NULL;

      ifp = (struct mpls_interface *)
        mpls_if_lookup ((u32)(dst2->dev->ifindex));
      if ((ifp == NULL) || (ifp->vrf == NULL))
        {
          PRINTK_ERR ("Error : Invalid outgoing interface %d\n",
                      dst2->dev->ifindex);
          kfree_skb (skb);
          return (FAILURE);
        }
    }

  /* ARP for neighbour entry. */
  if (dst2->neighbour == NULL)
    {
      arp_bind_neighbour (dst2);
      /* Valid arp entry does not exist. Drop the packet */
      if (!dst2->neighbour || !(dst2->neighbour->nud_state&NUD_VALID))
        {
          kfree_skb (skb);
          return (FAILURE);
        }
    }
  /* Valid arp entry does not exist. Drop the packet */
  else if (!(dst2->neighbour->nud_state&NUD_VALID))
    {
      kfree_skb (skb);
      return (FAILURE);
    }
  
  /* Set DST in sk buffer object. */
  skb->dst = (struct dst_entry *)rt2;
  
  /* Forward packet via outgoing device. */
  if (dst2->neighbour != NULL)
    {
      write_lock_bh (&dst2->neighbour->lock);
      dst2->neighbour->output = dst_neigh_output;
      write_unlock_bh (&dst2->neighbour->lock);
      result = dst2->input (skb);
    }
  else
    {
      result = FAILURE;
      kfree_skb (skb);
    }

  /* Release dst. */
  if (atomic_read (&dst2->__refcnt) > 1)
    dst_release (dst2);
  rt_add (rt2);

  return result;
}

/*
  This function checks the ttl value in the given packet.
  If ttl is one or less, the packet should not be forwarded
*/
int
check_and_update_ttl (struct sk_buff *sk, struct shimhdr *shimhp, 
                      uint8 update_ttl)
{
  uint8 ttl = 0;
  
  /* check if pkt is labelled */
  if (shimhp)
    {
      ttl = MAX(get_ttl_net(shimhp->shim), 1) - 1;

      if ((ttl > 0) && (update_ttl))
        {
          /* copy to shim */
          set_ttl_net (shimhp->shim, ttl);
        }
    }
  else /* unlabelled packet */
    {
      if (egress_ttl == -1)
        {
          ttl = MAX (ip_hdr(sk)->ttl, 1) - 1;
	
          if ((ttl > 0) && (update_ttl))
            {
              /* decrement ip ttl and recompute ip checksum */
              ip_decrease_ttl (ip_hdr(sk));
            }
        }
      else
        ttl = egress_ttl;
    }

  return ttl;
}


int
mpls_tcp_output (struct sk_buff **pskb)
{
  int ret = -1;
  uint32 d_addr;
  struct ftn_entry *ftn_entry;

  d_addr = ip_hdr(*pskb)->daddr;

  if ((ipv4_is_loopback(d_addr)) || (is_myaddr (sock_net((*pskb)->sk), d_addr)))
    {
      return 1;
    }
  
  while (d_addr)
    {
      /* Find the FTN entry */
      ftn_entry = fib_best_ftnentry (fib_handle->ftn, *pskb, d_addr,
                                     RT_LOOKUP_OUTPUT, 0);
      if (! ftn_entry)
        {
          ret = 1;
          break;
        }
      d_addr = 0;
      
      ret = mpls_nhlfe_process_opcode (pskb, ftn_entry->nhlfe_list,
                                       MPLS_FALSE, MPLS_FALSE, 0, 0);
      switch (ret)
        {
        case FTN_LOOKUP:
          d_addr = ftn_entry->nhlfe_list->rt->rt_gateway;
          if (! d_addr)
            ret = 1;
          break;
          
        case LOCAL_IP_FWD:
          ret = 1;
          break;
          
        case MPLS_LABEL_FWD:
          /* Release old dst. */
          if ((*pskb)->dst)
            {
              dst_release ((*pskb)->dst);
              (*pskb)->dst = NULL;
            }
          
          /* Forward labelled packet. */
          mpls_pre_forward ((*pskb), ftn_entry->nhlfe_list->rt,
                            MPLS_LABEL_FWD);

          ret = 0;
          break;
          
        default :
          ret = -1;
        }
    } 
        
  if (ret < 0)
    ret = 1;
  
  return ret;
}

int
mpls_nhlfe_process_opcode (struct sk_buff **sk, struct nhlfe_entry *list,
                           uint8 check_for_ttl, uint8 router_alert_set,
			   uint8 ttl_val, int flags) 
{
  struct nhlfe_entry *nhlfe = NULL;
  struct shimhdr *shimhp = NULL;
  uint8 labelled = MPLS_FALSE;
  uint8 last_label = MPLS_FALSE;
  int fwd_type;
  LABEL label;
  sint32 opcode = INVALID;
  struct label_primitive *label_primitive;
  __u32 daddr;
  int ret;

  if (!sk || (*sk == NULL) || !list)
    {
      PRINTK_ERR ("mpls_nhlfe_opcode_process : Invalid parameters\n");
      return FAILURE;
    }

  /*
   * If its a labelled packet, then shim header starts from skb->data
   */
  if ((*sk)->network_header != (*sk)->data)
    {
      shimhp = (struct shimhdr *) (*sk)->data;

      if (!shimhp)
        return FAILURE;

      labelled = MPLS_TRUE;

      if (get_bos_net (shimhp->shim))
        last_label = MPLS_TRUE;
    }

  /* Find the best nhlfe entry */
  nhlfe = forwarder_find_best_nhlfe(list);
  label_primitive = nhlfe->primitive;

  if (!label_primitive)
    return FAILURE;


  for (; label_primitive; label_primitive = label_primitive->next)
    {
      switch (label_primitive->opcode)
        {
        case POP:
          if (!shimhp)
            return FAILURE;
	  
          label = get_label_net(shimhp->shim);

          PRINTK_DEBUG ("Popping label ...%d\n", (int)label);

          POPLABEL(*sk);
          
          /* reset shimhp if all labels have been popped */
          if (last_label)
            {
              shimhp = NULL;
              labelled = MPLS_FALSE;
            }
          else
            {
              shimhp = (struct shimhdr *) (*sk)->data;
            }
          opcode = POP;
          break;

        case POP_FOR_VPN:
          if (!shimhp)
            return FAILURE;

          if (check_for_ttl)
            {
              check_for_ttl = MPLS_FALSE;
              ret = check_and_update_ttl (*sk, shimhp, MPLS_TRUE);
              if (ret <= 0)
                {
                  /* Report failure */
                  return (FAILURE);
                }
            }

          label = get_label_net(shimhp->shim);

          PRINTK_DEBUG ("Popping label for VPN ... %d\n", (int)label);

          POPLABEL(*sk);

          if (last_label)
            labelled = MPLS_FALSE;

          opcode = POP_FOR_VPN;
          break;

        case DLVR_TO_IP:

          if (labelled)
            return FAILURE;

          if (check_for_ttl)
            {
              check_for_ttl = MPLS_FALSE;
              ret = check_and_update_ttl (*sk, shimhp, MPLS_TRUE);
              if (ret <= 0)
                {
                  /* Report failure */
                  return (FAILURE);
                }
            }

          opcode = DLVR_TO_IP;
          break;

        case PUSH:

          if (check_for_ttl)
            {
              check_for_ttl = MPLS_FALSE;
              ret = check_and_update_ttl (*sk, shimhp, MPLS_TRUE);
              if (ret <= 0)
                {
                  /* Report failure */
                  return (FAILURE);
                }
            }

	  if (flags & MPLSONM_FWD_OPTION_NOPHP)
            {
#ifdef HAVE_MPLS_OAM_ITUT           
	      label = MPLS_OAM_ITUT_ALERT_LABEL;
#else
	      label = IPV4EXPLICITNULL;
#endif
              push_label_out (*sk, label, 1); 
            }

          label = label_primitive->label;

          push_label_out (*sk, label, ttl_val);

          opcode = PUSH;
          labelled = MPLS_TRUE;
	  
          break;

        case PUSH_AND_LOOKUP:

          if (check_for_ttl)
            {
              check_for_ttl = MPLS_FALSE;
              ret = check_and_update_ttl (*sk, shimhp, MPLS_FALSE);
              if (ret <= 0)
                {
                  /* Report failure */
                  return (FAILURE);
                }
            }

          label = label_primitive->label;
          PUSHLABEL(*sk, label, ttl_val);

          opcode = PUSH_AND_LOOKUP;
          labelled = MPLS_TRUE;
          break;

	case PUSH_AND_LOOKUP_FOR_VC:
 	case PUSH_FOR_VC:

	  if (check_for_ttl)
            {
              check_for_ttl = MPLS_FALSE;
              ret = check_and_update_ttl (*sk, shimhp, MPLS_FALSE);
              if (ret <= 0)
                {		  
                  /* Report failure */
                  return (MPLS_RET_TTL_EXPIRED);
                }
            }

          label = label_primitive->label;
          PUSHLABEL(*sk, label, ttl_val); 

          opcode = PUSH_AND_LOOKUP_FOR_VC;
          labelled = MPLS_TRUE;
          break;
        
        case SWAP:
          if (!shimhp)
            return FAILURE;

          if (check_for_ttl)
            {
              check_for_ttl = MPLS_FALSE;
              ret = check_and_update_ttl (*sk, shimhp, MPLS_TRUE);
              if (ret <= 0)
                {
                  /* Report failure */
                  return (FAILURE);
                }
            }

          {
            LABEL newlabel;
            
            newlabel = label_primitive->label;
            label = get_label_net(shimhp->shim);

            PRINTK_DEBUG ("Swapping label %d with %d...\n",
                          (int)label, (int)newlabel);

            SWAPLABEL(*sk, newlabel);
          }
          opcode = SWAP;
          labelled = MPLS_TRUE;
          break;
      
        case FTN_LOOKUP: 

          /* added for BGP routes */
          if (check_for_ttl)
            {
              check_for_ttl = MPLS_FALSE;
              ret = check_and_update_ttl (*sk, shimhp, MPLS_FALSE);
              if (ret <= 0)
                {
                  /* Report failure */
                  return (FAILURE);
                }
            }

          opcode = FTN_LOOKUP;
          break;
          
        default:

          PRINTK_ERR ("Unknown opcode %d in label_primitive\n",
                      label_primitive->opcode);
          return FAILURE;
        }
    }

  /* If the opcode was PUSH_AND_LOOKUP, we need to do a second
      lookup */
   if ((opcode == PUSH_AND_LOOKUP) ||
       (opcode == PUSH_AND_LOOKUP_FOR_VC) ||
       (opcode == FTN_LOOKUP) ||
       (opcode == SWAP_AND_LOOKUP))
     return DO_SECOND_FTN_LOOKUP;

  /* get destination */
  daddr = ip_hdr(*sk)->daddr;

  if (opcode == INVALID)
    {
      return FAILURE;
    }
  /* get forwarding type */
  fwd_type = mpls_resolve_route (opcode, labelled, daddr);
  
  /* update ttl */
  if ((check_for_ttl) && (fwd_type != LOCAL_IP_FWD))
    {   
      ret = check_and_update_ttl (*sk, shimhp, MPLS_TRUE);
      if (ret <= 0)
        {
          /* Report failure */
          return (FAILURE);
        }
    }


  /*
   * If the top label was a ROUTERALERT label then we do all the processing
   * based on the label beneath the ROUTERALERT label, but finally we have
   * to push the ROUTERALERT label back
   */

  if ((router_alert_set) && (router_alert))
    {
      uint32 router_alert_label = get_label_net(router_alert);

      router_alert = 0;

      if (fwd_type == MPLS_LABEL_FWD)
        PUSHLABEL(*sk, router_alert_label, 0); 
    }
  return fwd_type;
}


int
mpls_resolve_route (sint32 opcode, uint8 labelled, uint32 daddr)
{
  if ((opcode == FTN_LOOKUP) || (opcode == PUSH_AND_LOOKUP))
    return FTN_LOOKUP;
      
  if (!labelled)
    {
      /* If this packet is meant for us */
      if (is_myaddr (&init_net, daddr))
        return LOCAL_IP_FWD;

      if (opcode == POP)
        return MPLS_IP_FWD;

      if ((opcode == POP_FOR_VPN) || 
          (opcode == DLVR_TO_IP))
        return MPLS_VPN_FWD;
    }

  return MPLS_LABEL_FWD;
}


int 
push_label_out (struct sk_buff *skb, LABEL label, uint8 ttl_val)
{
  u32 ttl, bos;
  struct shimhdr *shimhp = NULL;

  if ((skb->data - skb->head) >= (SHIM_HLEN + 16))
    {
      skb_push (skb, SHIM_HLEN);
      shimhp = (struct shimhdr *)skb->data;
      
      if (((uint8 *)skb->network_header) - ((uint8 *)skb->data) == SHIM_HLEN)
        {
          /* this is the only label */
          bos = 1;
	  if (ttl_val)
            ttl = ttl_val;
          if (ingress_ttl == -1)
            ttl = ip_hdr(skb)->ttl;
          else
            ttl = ingress_ttl;
        }
      else
        {
	  if (! ttl_val)
	    ttl = get_ttl_net (((struct shimhdr *)((uint8 *)shimhp + 
                                                 SHIM_HLEN))->shim);
	  else 
            ttl = ttl_val;

          bos = 0;
        }
      
      set_mpls_shim_net (shimhp, label, 0, bos, ttl);

    }
  else
    {
      /* check if there is tail room */
      if (skb->end - skb->tail >= SHIM_HLEN)
        {
          skb->tail += SHIM_HLEN;
          skb->data += SHIM_HLEN;
          skb->network_header += SHIM_HLEN;
	  
          /* Move the header enough */
          memmove (skb->network_header, skb->network_header - SHIM_HLEN, skb->data - skb->tail);
          skb_push (skb, SHIM_HLEN);
          shimhp = (struct shimhdr *)skb->data;
        }  
      else
        {
          return MPLS_FALSE;
        }
    }
  
  if (((uint8 *)skb->network_header) - ((uint8 *)skb->data) == SHIM_HLEN)
    {
      /* this is the only label */
      bos = 1;
      if (ttl_val)
        ttl = ttl_val;
      else if (ingress_ttl == -1)
        ttl = ip_hdr(skb)->ttl;
      else
        ttl = ingress_ttl;
    }
  else
    {
      if (! ttl_val)
        ttl = get_ttl_net (((struct shimhdr *)((uint8 *)shimhp +
                                               SHIM_HLEN))->shim);
      else
        ttl = ttl_val;
      bos = 0;
    }
      
  set_mpls_shim_net (shimhp, label, 0, bos, ttl);
  
  return MPLS_TRUE;
}


/* PUSH Virtual Circuit label on a received frame. If the opcode in the
   FTN entry is PUSH_AND_LOOKUP_FOR_VC, lookup for second label in the
   GLOBAL FTN table and find a second label to push, based on the 
   nexthop in the VC FTN entry.
   This function will return the outmost label's associated rtable entry . */
struct rtable *
mpls_push_vc_label (struct sk_buff **sk, struct ftn_entry *ftn)
{
  struct sk_buff *pskb;
  struct label_primitive *l_prim;
  struct shimhdr *shimhp;
  struct ftn_entry *ftn2;
  struct rtable *rt;
  uint8 labels_to_push;
  uint32 key;

  /* This should NEVER happen. */
  if (ftn->nhlfe_list == NULL)
    {
      PRINTK_ERR ("mpls_vc_rcv: FTN entry on has no NHLFE !!\n");
      return NULL;
    }

  /* Get rtable entry. */
  rt = ftn->nhlfe_list->rt;
  if (rt == NULL)
    {
      PRINTK_ERR ("mpls_vc_rcv: FTN entry has no rt !!\n");
      return NULL;
    }

  /* Get first label primitive. */
  l_prim = ftn->nhlfe_list->primitive;

  /* Labels to push. */
  if (l_prim->opcode == PUSH_FOR_VC)
    labels_to_push = 1;
  else if (l_prim->opcode == PUSH_AND_LOOKUP_FOR_VC)
    labels_to_push = 2;
  else
    {
      PRINTK_ERR ("mpls_vc_rcv: Invalid opcode for FTN entry\n");
      return NULL;
    }

  pskb = skb_unshare (*sk, GFP_ATOMIC);
  if (pskb == NULL)
    {
      PRINTK_ERR ("mpls_vc_rcv: Failed to make local copy of skb\n");
      return NULL;
    }

  *sk = pskb;

  /* Check if there's enough space to PUSH label. */
  if (((int)(*sk)->mac_header - (int)(*sk)->head) < ((SHIM_HLEN * labels_to_push) +
                                        ETH_HLEN2))
    {
      int req_space = (SHIM_HLEN * labels_to_push) + ETH_HLEN2 -
        ((int)(*sk)->mac_header - (int)(*sk)->head);

      PRINTK_DEBUG ("mpls_vc_rcv: Not enough head room %d to push "
                    "VC label. Checking tail.\n", ((int)(*sk)->mac_header -
                                                   (int)(*sk)->head));
      if ((*sk)->end - (*sk)->tail > req_space)
        {
          memmove ((*sk)->mac_header + req_space, (*sk)->mac_header,
                   (*sk)->tail - (*sk)->mac_header); 
          (*sk)->mac_header += req_space;
          (*sk)->tail += req_space;
          (*sk)->data += req_space;
          (*sk)->network_header += req_space;
        }
      else
        {
          struct sk_buff *newsk = NULL;
          int newheadroom; 
          
          newheadroom = ETH_HLEN + ETH_HLEN2 + (SHIM_HLEN * labels_to_push);
          
          PRINTK_DEBUG ("mpls_vc_rcv: Not enough room in sk_buff for "
                        "label(s). Creating new sk_buff\n");
          newsk = skb_copy_expand (*sk, newheadroom, 
                                   ETH_HLEN, GFP_ATOMIC);
          if (newsk == NULL)
            {
              PRINTK_ERR ("mpls_vc_rcv: Could not allocate extra space "
                          "sk buff\n");
              return NULL;
            }
          
          newsk->mac_header = newsk->data - ETH_HLEN;
          memcpy (newsk->mac_header, (*sk)->mac_header, 
                  sizeof (struct ethhdr));
          kfree_skb (*sk);
          *sk = newsk;
        }
    }
  
  /*
   * First add VC label.
   */
  
  /* Make space for one label and the internal ethernet header. */
  skb_push (*sk, ETH_HLEN + SHIM_HLEN);
  
  /* Set VC label and mark is as the bottom-of-stack label. EXP bits
     are not set. TTL for this label is set to 2. */
  shimhp = (struct shimhdr *)(*sk)->data;
  set_mpls_shim_net (shimhp,l_prim->label, 0, 1, 2); 
  
  if (labels_to_push == 1)
    return rt;

  /*
   * Lookup second label, and PUSH it.
   */
  
  /* Key for lookup. */
  key = ftn->nhlfe_list->rt->rt_gateway;
  if (key == 0)
    {
      PRINTK_ERR ("mpls_vc_rcv: Key for second label lookup is invalid !!\n");
      return NULL;
    }

  /* Lookup second ftn. */
  ftn2 = fib_best_ftnentry (fib_handle->ftn, (*sk), key, RT_LOOKUP_INPUT, 1);

  /* Match tunnel_ftnix */
  if (ftn->tunnel_ftnix != 0)
    {
      while (ftn2)
        {
          if (ftn2->ftn_ix == ftn->tunnel_ftnix)
            break;

          ftn2 = ftn2->next;
        }
  }
  
  if (ftn2 == NULL)
    {
      PRINTK_ERR ("mpls_vc_rcv: Second FTN entry not found in GLOBAL FTN "
                  "for key %d.%d.%d.%d\n", NIPQUAD (key));
      return NULL;
    }

  /* This should NEVER happen. */
  if (ftn2->nhlfe_list == NULL)
    {
      PRINTK_ERR ("mpls_vc_rcv: Second FTN entry on has no NHLFE !!\n");
      return NULL;
    }

  /* Get rt entry. */
  rt = ftn2->nhlfe_list->rt;
  if (rt == NULL)
    {
      PRINTK_ERR ("mpls_vc_rcv: Second FTN entry has no rt !!\n");
      return NULL;
    }

  /* Make space for second label. */
  skb_push ((*sk), SHIM_HLEN);

  /* Set second label. EXP bits are not set. This is not the 
     bottom-of-stack label. TTL should be MAX. */
  l_prim = ftn2->nhlfe_list->primitive;
  shimhp = (struct shimhdr *)(*sk)->data;
  set_mpls_shim_net (shimhp, l_prim->label, 0, 0, MAXTTL);

  if ((*sk)->dev->mtu < (*sk)->len)
    {
      PRINTK_ERR ("mpls_push_vc_label : Dropping VC packet. Packet size"
                  " %d is more than VC  mtu size %d", 
                  (*sk)->len, (*sk)->dev->mtu); 
      return NULL;
    }

  return rt;
}

/* POP Virtual Circuit specific label. This will also set the outgoing
   interface based on the NHLFE entry. */
int
mpls_pop_and_fwd_vc_label (struct sk_buff *sk, struct rtable *rt,
                           struct shimhdr *shimhp)
{
  int ret;

  /* If no shim, return. */
  if (shimhp == NULL)
    {
      PRINTK_ERR ("mpls_pop_and_fwd_vc_label: No shim header !!\n");
      return FAILURE;
    }

  /* If no rt, return. */
  if (rt == NULL)
    {
      PRINTK_ERR ("mpls_pop_and_fwd_vc_label: No rt entry !!\n");
      return FAILURE;
    }

  /* There should only be one label left. */
  if (! get_bos_net (shimhp->shim))
    {
      PRINTK_ERR ("mpls_pop_and_fwd_vc_label: Label is not the "
                  "bottom-of-stack label !!\n");
      return FAILURE;
    }

  /*
    Data pointer is currently pointing at the outer label. Move it
    forward by SHIM_HLEN and it will point to the inner ethernet frame.
  */
  skb_pull (sk, SHIM_HLEN);
  sk->mac_header = sk->data;
  sk->dev = rt->u.dst.dev;
  if (sk->dev->mtu < sk->len - ETH_HLEN)
    {
      PRINTK_ERR ("mpls_pop_and_fwd_vc_label : Dropping VC packet. Packet size"
                  " %d is more than outgoing device's mtu size %d", 
                  sk->len - ETH_HLEN, sk->dev->mtu); 
      return FAILURE;
    }

  /* Pass on frame to device. */
  ret = dev_queue_xmit (sk);
  if (ret != SUCCESS)
    {
      PRINTK_ERR ("mpls_pop_and_fwd_vc_label: Failed to send out frame "
                  "via device !!\n");
      return FAILURE;
    }

  return SUCCESS;
}

/* Function to receive data on an interface bound to a Virtual Circuit. */
int
mpls_vc_rcv (struct sk_buff *sk, struct net_device *dev, struct ftn_entry *ftn,
             struct packet_type *pt)
{
  struct rtable *rt;

  /* If packet is a raw packet without an ethernet header, drop it. */
  if (sk->mac_header == sk->network_header)
    {
      kfree_skb (sk);
      PRINTK_ERR ("mpls_vc_rcv : Insane sk_buff received\n");
      return FAILURE;
    }

  /* Push on a Virtual Circuit label. */
  rt = mpls_push_vc_label (&sk, ftn);
  if (rt == NULL)
    {
      kfree_skb (sk);
      PRINTK_ERR ("mpls_vc_rcv : Could not PUSH Virtual Circuit label "
                  "for Ethernet frame received on interface %s\n",
                  dev->name);
      return FAILURE;
    }

  /* Forward labeled packet out. */
  return mpls_pre_forward (sk, rt, MPLS_VC_FWD);
}

void
set_mpls_shim_net (struct shimhdr *shimhp , u32 label, 
                   u32 exp, u32 bos, u32 ttl)
{
  shimhp->shim &= ~(shimhp->shim);
  shimhp->shim |= (0x000000ff & ttl);
  shimhp->shim |= (0x00000001 & bos) << 8;
  shimhp->shim |= (0x00000007 & exp) << 9; 
  shimhp->shim |= (label << 12);
  shimhp->shim = htonl (shimhp->shim);
}

/** @brief Function to send packets to OAMD
 
    @param skb        - Pointer to sk_buff structure
    @param flags      - pkt type
    @param ifp        - pointer to interface structure
    @input1           - cc type
    @input2           - encapsulation type
    @return none
 */
void
mpls_oam_packet_process (struct sk_buff *skb, struct mpls_interface *ifp,
                         uint8 input1, uint8 input2, uint32 label_stack_depth,
                         uint32 *label_stack)
{
  struct mpls_oam_ctrl_data ctrl_data;
  struct udphdr *udph = NULL;
  int pkt_type, i;
  int skb_room = 0, req_skb_room = 0;
  int head_room = 0, tail_room = 0;

  memset (&ctrl_data, 0,
          sizeof (struct mpls_oam_ctrl_data));

  /* If packet contains PW ACH, Remove the same*/
  if (input1 == CC_TYPE_1)
  {
    memmove((skb)->mac_header+=PW_ACH_LEN, (skb)->mac_header, ETH_HLEN);
    skb_pull((skb), PW_ACH_LEN);
  }

  /* if encapsulatin is not RAW BFD update the skb_room required to be
   * ip/udp header length minus, since the same is not required to be 
   * send to the control plane */
  if (input2 != BFD_CV_RAW_ENCAP)
    {
      skb_room = (ip_hdr(skb)->ihl << 2) + 8 -
          sizeof (struct mpls_oam_ctrl_data);

      skb->data = (void *)ip_hdr(skb);
      skb->len = ntohs (ip_hdr(skb)->tot_len);
      udph = (struct udphdr*)((uint8 *)(ip_hdr(skb))+(ip_hdr(skb)->ihl << 2));
    }
  else
    {
      skb->data = (void *)ip_hdr(skb);
      skb_room = skb_room - (sizeof (struct mpls_oam_ctrl_data));
    }

  /* req_skb_room contains the skb room required for the netlink msg hdr too */
  req_skb_room = (0 - skb_room) + NLMSG_ALIGN (sizeof (struct nlmsghdr));


  /* If existing skb head room is positive, that is ip/udp hdr space is
   * not enough for pushing the oam ctrl data */
  if (req_skb_room > 0)
    {
      head_room = (int)(skb)->mac_header - (int)(skb)->head;
      tail_room = (int)(skb)->end - (int)(skb)->tail;

      /* If head of the skb is not enough */
      if (head_room < (req_skb_room))
        {
          /* if the tail room of the skb is also not enough */
          if (tail_room < (req_skb_room))
            {
              /* expand the skb to the required size */
              struct sk_buff *old;
                old = (skb);
                PRINTK_DEBUG ("mpls_oam_packet_process : Not enough room "
                   "in sk_buff for sending oam packet, creating new sk_buff\n");
                (skb) = skb_copy_expand(old,ETH_HLEN+req_skb_room,
                                        16,GFP_ATOMIC);
                if((skb) == NULL) { (skb) = old; return; }
                  (skb)->mac_header = (skb)->data - ETH_HLEN;
                    kfree_skb(old);
            }
          else
            {
              /* if tail room can be used, move the data and free the head
               * room */
              memmove((skb)->mac_header+req_skb_room,
                      (skb)->mac_header, (skb)->tail - (skb)->mac_header);
              (skb)->tail    += req_skb_room;
              (skb)->data    += req_skb_room;
              (skb)->network_header  += req_skb_room;
              (skb)->mac_header += req_skb_room;
              /* re-adjust the udph pointer, so that can be used below */
              udph = (struct udphdr*)\
                     ((uint8 *)(ip_hdr(skb))+(ip_hdr(skb)->ihl << 2));
            }
        }
    }

  ctrl_data.input1 = input1;
  ctrl_data.input2 = input2;

  /* PW ACH channel type is bfd or the udp destination port is 
   * MPLS_BFD_UDP_PORT it is a BFD OAM packet */
  if ((input2 == BFD_CV_RAW_ENCAP) || 
      (ntohs (udph->dest) == MPLS_BFD_UDP_PORT))
    {
      pkt_type = BFD_OAM;
      if (!input2)
        ctrl_data.input2 = BFD_CV_IP_UDP_ENCAP;
    }
  else /* else it is MPLS OAM packet */
    {
      pkt_type = MPLS_OAM;
    }

  /* fill the ctrl data to be sent to the control plane */
  memcpy (&ctrl_data.src_addr, &ip_hdr(skb)->saddr, 4);
  ctrl_data.in_ifindex = ifp->ifindex;
  ctrl_data.label_stack_depth = label_stack_depth;
  
  if (skb_room > 0)
    skb_pull (skb, skb_room);
  else if (skb_room < 0)
    skb_push (skb, (0 - skb_room));

  memcpy (skb->data, &ctrl_data.src_addr, 4);
  memcpy (skb->data + 4, &ctrl_data.in_ifindex, 4);
  memcpy (skb->data + 8, &ctrl_data.label_stack_depth, 4);
  for (i = 0; i < label_stack_depth; i++)
    {
      ctrl_data.label_stack[i] = label_stack[i];
      memcpy (skb->data + 12 + (i * 4), ctrl_data.label_stack, 4);
    }
  memcpy (skb->data + 28, &ctrl_data.input1, 4);
  memcpy (skb->data + 32, &ctrl_data.input2, 4);
  /* send the packet to the control plane over the netlink socket */
  mpls_netlink_send (skb, &ctrl_data, pkt_type);
  }

