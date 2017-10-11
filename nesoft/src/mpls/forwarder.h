/* Copyright (C) 2001-2003  All Rights Reserved.  */
 
#ifndef _PACOS_MPLS_FWDER_H
#define _PACOS_MPLS_FWDER_H

/* Hash defines for forwarder module */

/* Reserved label values. Section 2.1 of RFC 3032. */
#define IPV4EXPLICITNULL  0
#define ROUTERALERT       1
#define IPV6EXPLICITNULL  2
#define IMPLICITNULL      3
#define RESERVEDLABEL(i)  ((i) >= 4 && (i) < 16)
#define FIRSTUNRESERVED   16
#define MPLS_OAM_ITUT_ALERT_LABEL 14
/* First label we can advertise.  */
#define START_LABEL       FIRSTUNRESERVED

#define MPLS_OAM_FDI_PROCESS_MESSAGE 2

/* First label we can advertise.  */
#define START_LABEL       FIRSTUNRESERVED

#define MPLS_OAM_UDP_PORT 3503
#define MPLS_BFD_UDP_PORT 3784
#define MPLS_LDP_UDP_PORT 646
#define MPLS_LMP_UDP_PORT 701

/* Macros to PUSH/POP/SWAP labels on an MPLS packet sk */

/* POPLABEL takes as argument the pointer to the sk_buff structure
   which contains the MPLS packet. */
#define POPLABEL(sk)\
{\
    uint8 ttl;\
    char *retp;\
    struct shimhdr *shimhp;\
    shimhp = (struct shimhdr *)((uint8 *)(sk)->mac_header + ETH_HLEN);\
    /*\
     * We should have at least one label to POP.\
     */\
    MPLS_ASSERT((uint8 *)ip_hdr(sk) != (uint8 *)shimhp);\
    PRINTK_DEBUG ("POPLABEL : POPed label %d\n", \
           (int)get_label_net(shimhp->shim));\
    if (egress_ttl == -1)\
      ttl = get_ttl_net(shimhp->shim);\
    else\
      ttl = egress_ttl;\
    if(get_bos_net(shimhp->shim))\
      {\
         /* Check if the Packet contains PW ACH header, just before \
          * the bottom most label */\
         if (IS_PW_ACH(*((uint8 *)(shimhp) + SHIM_HLEN)))\
          {\
            /* If PW ACH, then we don't have any IP/UDP Header when the \
             * channel type is BFD, so skip the ttl copying to IP header \
             * in this case */\
            if (ntohs (get_channel_type_net(*(((uint32 *)(shimhp) + 1)))) \
                                                          != MPLS_PW_ACH_BFD)\
              {\
                MPLS_ASSERT((uint8 *)ip_hdr(sk) - \
                    (uint8 *)shimhp == SHIM_HLEN); \
                /* no more labels */\
                /*\
                 * copy the ttl from the label stack to the IP ttl field\
                 */ \
                if (lsp_model == LSP_MODEL_UNIFORM)\
                  {\
                    ip_hdr(sk)->ttl = ttl;\
                    PRINTK_DEBUG ("POPLABEL : ttl value [%d] "\
                        "from the BOS label, set in the IP header \n", ttl);\
                  }\
                else if (IPV4EXPLICITNULL != (int) get_label_net(shimhp->shim))\
                  {\
                    /* \
                       In PIPE model with PHP enabled, Penultimate hop SHOULD \
                       NOT decrement the IP TTL. So the IP TTL is incremented\
                       here to offset the effect of a TTL decrementation that\
                       happens during subsequent IP forwarding on this hop\
                       */ \
                    ip_hdr(sk)->ttl++;\
                  }\
                ip_send_check(ip_hdr(sk));\
              }\
          }\
         /* If Not PW ACH */\
         else\
          {\
            MPLS_ASSERT((uint8 *)ip_hdr(sk) - \
                (uint8 *)shimhp == SHIM_HLEN); \
            /* no more labels */\
            /*\
             * copy the ttl from the label stack to the IP ttl field\
             */ \
            if (lsp_model == LSP_MODEL_UNIFORM)\
              {\
                ip_hdr(sk)->ttl = ttl;\
                PRINTK_DEBUG ("POPLABEL : ttl value [%d] "\
                    "from the BOS label, set in the IP header \n", ttl);\
              }\
            else if (IPV4EXPLICITNULL != (int) get_label_net(shimhp->shim))\
              {\
                /* \
                   In PIPE model with PHP enabled, Penultimate hop SHOULD NOT\
                   decrement the IP TTL. So the IP TTL is incremented\
                   here to offset the effect of a TTL decrementation that\
                  happens during subsequent IP forwarding on this hop\
                   */ \
                ip_hdr(sk)->ttl++;\
              }\
            ip_send_check(ip_hdr(sk));\
          }\
      }\
    else\
      {\
         /*\
          * removed one label\
          */\
          shimhp = (struct shimhdr *)((uint8 *)shimhp + SHIM_HLEN);\
        /*\
         * copy the ttl from the previous top-of-label to the new top-of-label,\
         * only if the ttl in the new top-of-label is greater than the ttl in\
         * the top-of-label */\
          if (get_ttl_net(shimhp->shim) > ttl)\
            {\
              set_ttl_net(shimhp->shim, ttl);\
            }\
      }\
    memmove((sk)->mac_header+=SHIM_HLEN, (sk)->mac_header, ETH_HLEN); \
    retp = skb_pull((sk), SHIM_HLEN);\
    MPLS_ASSERT(retp != NULL);\
}

/* This macro has the effect of removing all labels from the label
   stack. After this the 'sk' will contain an unlabelled packet.  */
#define POPALLLABELS(sk)\
{\
    char *retp;\
    struct shimhdr *shimhp = \
        (struct shimhdr *)((uint8 *)(sk)->mac_header + ETH_HLEN);\
    uint32 stack_size = (uint8*)ip_hdr(sk) - \
                        (uint8*)shimhp;\
    uint8 ttl; \
    /*\
     * We should have at least one label to POP.\
     */\
    MPLS_ASSERT((uint8 *)ip_hdr(sk) != (uint8 *)shimhp);\
    /*\
     * no more labels.\
     */\
    memmove((sk)->mac_header+=stack_size, (sk)->mac_header, ETH_HLEN); \
    ttl = get_ttl_net(shimhp->shim);\
    ip_hdr(sk)->ttl = ttl; \
    ip_send_check (ip_hdr(sk)); \
    retp = skb_pull((sk), stack_size);\
    MPLS_ASSERT(retp != NULL);\
}

/**@brief - This macro pushes the PW ACH Header with channel type set to 
 *  channel_type passed as an argument. 
 *
 * @param sk - Pointer to Sk Buff structure where PW ACH to be prepended
 * @channel_type - Channel type to be set in the PW ACH Header
 */

#define PUSH_PW_ACH(sk, channel_type)\
do{\
    struct pwachhdr *pwachp = NULL;\
    /*\
     * check if we have room to add an extra label in the sk_buff. If \
     * not we have to expand the sk_buff.\
     */\
    if((int)(sk)->mac_header - (int)(sk)->head >= PW_ACH_LEN)\
    {\
        /*\
         * we have enough headroom to push a label.\
         */\
        PRINTK_DEBUG ("PUSH_PW_ACH : Using headroom "\
               "for pushing the label\n");\
    }\
    else if((int)(sk)->end - (int)(sk)->tail >= PW_ACH_LEN)\
    {\
         PRINTK_DEBUG ("PUSH_PW_ACH : Using tailroom "\
                "for pushing the label\n");\
         memmove((sk)->mac_header+PW_ACH_LEN, (sk)->mac_header, (sk)->tail - \
                 (sk)->mac_header);\
         (sk)->tail    += PW_ACH_LEN;\
         (sk)->data    += PW_ACH_LEN;\
         (sk)->network_header  += PW_ACH_LEN;\
         (sk)->mac_header += PW_ACH_LEN;\
    }\
    else\
    {\
        /*\
         * no room in this sk_buff, we have to create a new one with enough\
         * room for one more label.\
         */\
         struct sk_buff *old;\
         old = (sk);\
         PRINTK_DEBUG ("PUSH_PW_ACH : Not enough room "\
                "in sk_buff for the label, creating new sk_buff\n");\
         (sk) = skb_copy_expand(old,ETH_HLEN+PW_ACH_LEN,16,GFP_ATOMIC);\
         if((sk) == NULL) { (sk) = old; return -ENOMEM; }\
         (sk)->mac_header = (sk)->data - ETH_HLEN;\
         kfree_skb(old);\
    }\
    memmove((sk)->mac_header-PW_ACH_LEN, (sk)->mac_header, ETH_HLEN);\
    skb_push((sk), PW_ACH_LEN);\
    (sk)->mac_header -= PW_ACH_LEN;\
    (sk)->network_header -= PW_ACH_LEN;\
    pwachp = (struct pwachhdr *)((sk)->mac_header + ETH_HLEN);\
    memset (pwachp, 0, sizeof (struct pwachhdr));\
    /*\
     * now copy the PW ACH at  pwachp, it should be \
     * put in network byte order.\
     */\
    set_first_nibble_net(pwachp->pwach, 0x1);\
    set_channel_type_net(pwachp->pwach, channel_type);\
    PRINTK_DEBUG ("PUSH_PW_ACH : PUSHed PW ACH "\
           "%d\n", (int)pwachp->pwach);\
}while(0);

/* PUSHLABEL takes the first argument as the pointer to the sk_buff
  structure which contains the MPLS packet, the second argument is the
  'label' to be pushed (not the complete shim header), in host byte
  order. */
#define PUSHLABEL(sk, label, ttl_val)\
do{\
    uint8 ttl;\
    /*\
     * check if we have room to add an extra label in the sk_buff. If \
     * not we have to expand the sk_buff.\
     */\
    if((int)(sk)->mac_header - (int)(sk)->head >= SHIM_HLEN)\
    {\
        /*\
         * we have enough headroom to push a label.\
         */\
        PRINTK_DEBUG ("PUSHLABEL : Using headroom "\
               "for pushing the label\n");\
    }\
    else if((int)(sk)->end - (int)(sk)->tail >= SHIM_HLEN)\
    {\
         PRINTK_DEBUG ("PUSHLABEL : Using tailroom "\
                "for pushing the label\n");\
         memmove((sk)->mac_header+SHIM_HLEN, (sk)->mac_header, (sk)->tail - \
                 (sk)->mac_header);\
         (sk)->tail    += SHIM_HLEN;\
         (sk)->data    += SHIM_HLEN;\
         (sk)->network_header  += SHIM_HLEN;\
         (sk)->mac_header += SHIM_HLEN;\
    }\
    else\
    {\
        /*\
         * no room in this sk_buff, we have to create a new one with enough\
         * room for one more label.\
         */\
         struct sk_buff *old;\
         old = (sk);\
         PRINTK_DEBUG ("PUSHLABEL : Not enough room "\
                "in sk_buff for the label, creating new sk_buff\n");\
         (sk) = skb_copy_expand(old,ETH_HLEN+SHIM_HLEN,16,GFP_ATOMIC);\
         if((sk) == NULL) { (sk) = old; return -ENOMEM; }\
         (sk)->mac_header = (sk)->data - ETH_HLEN;\
         kfree_skb(old);\
    }\
      memmove((sk)->mac_header-SHIM_HLEN, (sk)->mac_header, ETH_HLEN);\
      skb_push((sk), SHIM_HLEN);\
      (sk)->mac_header -= SHIM_HLEN;\
      shimhp = (struct shimhdr *)((sk)->mac_header + ETH_HLEN);\
      /*\
       * now copy the label at  shimhp, it should be \
       * put in network byte order.\
       */\
      set_label_net(shimhp->shim, label);\
      set_exp_net(shimhp->shim, 0);\
      PRINTK_DEBUG ("PUSHLABEL : PUSHed label "\
             "%d\n", (int)label);\
      /*\
       * if this is the last/only label in the stack set the BOS bit.\
       * */ \
      if(((uint8 *)(sk)->network_header - (uint8 *)shimhp == SHIM_HLEN))\
       {\
         /*\
          * Section 2.4.1 RFC 3032\
          * The TTL field of a plain IP packet is decremented\
          * in the forwarder.\
          */ \
          if (ttl_val)\
            ttl = ttl_val;\
          else if (propagate_ttl == TTL_PROPAGATE_ENABLED)\
            ttl = ip_hdr(sk)->ttl;\
          else if (ttl_val)\
            ttl = ttl_val;\
          else if (ingress_ttl != -1)\
            ttl = ingress_ttl;\
          else\
            ttl = MAX_TTL;\
          set_bos_net(shimhp->shim, 1);\
         /* \
          * set the ttl value equal to the {TTL value in the \
          * layer 3 header minus one} or {zero} whichever is greater.\
          */ \
          set_ttl_net(shimhp->shim, ttl);\
          /*\
           * this was an unlabelled IP datagram onto which we have to push\
           * a label. So we do the MILDS check here\
           */\
          /*\
           * See the discussion on "Maximum Initially Labelled \
           * Datagram Size" in section 3.2 of RFC 3032\
           */\
          if(milds && !(ntohs(ip_hdr(sk)->frag_off) & IP_DF) &&\
                  (((sk)->len - SHIM_HLEN) > milds))\
          {\
              PRINTK_DEBUG ("PUSHLABEL : Unlabelled "\
                     "IP datagram received with size [%d] greater than the "\
                     "'Maximum Initially Labelled Datagram Size [%d]'\n",\
                     ((sk)->len - SHIM_HLEN), (int)milds);\
                     max_frag_size = milds + SHIM_HLEN;\
          }\
        }\
        else\
        {\
           /*\
            * copy the ttl value from the previous top of the stack \
            * label.\
            */ \
           ttl = \
           get_ttl_net(((struct shimhdr *)((uint8 *)shimhp + \
                                           SHIM_HLEN))->shim);\
           set_ttl_net(shimhp->shim, ttl);\
           set_bos_net(shimhp->shim, 0);\
        }\
}while(0);

/* The first argument is the pointer to the 'struct eth_frame' which
  contains the MPLS packet, the second argument is the 'label' (not
  the complete shim header) to be swapped with the top-of-stack label,
  in host byte order. */
#define SWAPLABEL(sk, newlabel)\
{\
   uint8 ttl;\
   struct shimhdr *shimhp = \
        (struct shimhdr *)((uint8 *)(sk)->mac_header + ETH_HLEN);\
   /*\
    * for swap we should have at least one label present in the \
    * packet.\
    */ \
   MPLS_ASSERT((uint8 *)ip_hdr(sk) != (uint8 *)shimhp);\
   MPLS_ASSERT((uint8 *)ip_hdr(sk) - \
          (uint8 *)shimhp >= SHIM_HLEN);\
   ttl = get_ttl_net(shimhp->shim);\
   /*\
    * now copy the label at  shimhp. This will\
    * overwrite the old label (swap in effect).\
    */ \
   PRINTK_DEBUG ("SWAPLABEL : SWAPped label "\
          "%u with %lu\n", \
          get_label_net(shimhp->shim),(newlabel));\
          set_label_net(shimhp->shim, (newlabel));\
          set_ttl_net(shimhp->shim, ttl);\
}\

#endif /* _PACOS_MPLS_FWDER_H */
