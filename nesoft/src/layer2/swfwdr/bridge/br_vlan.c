/* Copyright 2003  All Right Reserved. */
/*
  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version
  2 of the License, or (at your option) any later version.

*/

/* This module contains routines to support a VLAN aware bridge. */
#include <linux/autoconf.h>
#include "br_vlan.h"
#include "br_types.h"
#include "bdebug.h"
#include <linux/etherdevice.h>
#ifndef ETH_P_8021Q
#define ETH_P_8021Q 0x8100
#endif
#include "br_vlan_api.h"

extern unsigned char group_addr[6];
extern unsigned char rpvst_group_addr[6];
extern unsigned char cfm_ccm_addr[6];
extern unsigned char cfm_ltm_addr[6];
/* This function gets the vid from the tagged frame */


vid_t
br_vlan_get_vid_from_frame (unsigned short proto, struct sk_buff *skb)
{
  struct vlan_header *vlan_hdr;
  struct apn_bridge_port * port;

  port = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);

  if (proto == ETH_P_8021Q_CTAG
      && (port->port_type == PRO_NET_PORT
          || port->port_type == CUST_NET_PORT))
    vlan_hdr = (struct vlan_header *) (skb->data + sizeof (struct vlan_header));
  else
    vlan_hdr = (struct vlan_header *)skb->data;

  return (ntohs(vlan_hdr->vlan_tci) & VLAN_VID_MASK);

}

/* TODO */

unsigned char
br_vlan_get_ingress_priority (struct apn_bridge *br, unsigned short vlan_tag)
{
  /* FUTURE ingress_priority_map should be defined per bridge */
  return (vlan_tag >> 13) & VLAN_USER_PRIORITY_MASK;
}

unsigned short
br_vlan_get_egress_priority (vid_t vlan_tag)
{
  unsigned short cfi = 0;

  unsigned short vlan_tci = 0;
  unsigned short priority = DEFAULT_USER_PRIORITY;  /* To get priority from user */
  /* FUTURE eggress_priority_map should be defined per bridge */

  vlan_tci |= (priority << 13);
  vlan_tci |= ((cfi << 12));
  vlan_tci |= (( vlan_tag & 0x0fff));

  return vlan_tci;
}

br_frame_t
br_cvlan_get_frame_type (struct sk_buff *skb)
{
  br_frame_t frame_type;
  struct ethhdr *eth = (struct ethhdr *)skb_mac_header (skb);

  if (ntohs(eth->h_proto) == ETH_P_8021Q)
    {
      frame_type = (br_vlan_get_vid_from_frame (ETH_P_8021Q, skb) == VLAN_NULL_VID) ?
                    PRIORITY_TAGGED : TAGGED;
    }
  else
    {
      frame_type = UNTAGGED;
    }

  return frame_type;
}


/* This function returns the frame type associated with the frame */

br_frame_t
br_vlan_get_frame_type (struct sk_buff *skb,
                        unsigned short proto)
{
  br_frame_t frame_type;
  struct vlan_header *vlan_hdr;
  struct apn_bridge_port * port;
  struct ethhdr *eth = (struct ethhdr *)skb_mac_header (skb);

  port = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);

  if (port != NULL
      && proto == ETH_P_8021Q_CTAG
      && (port->port_type == PRO_NET_PORT
          || port->port_type == CUST_NET_PORT))
    {
      vlan_hdr = (struct vlan_header *)skb->data;

      if (ntohs (vlan_hdr->encapsulated_proto) == ETH_P_8021Q_CTAG)
        {
          frame_type = (br_vlan_get_vid_from_frame (proto, skb)
                                                  == VLAN_NULL_VID) ?
                        PRIORITY_TAGGED : TAGGED;
        }
      else
        frame_type = UNTAGGED;
    }
  else if (ntohs(eth->h_proto) == proto)
    {
      frame_type = (br_vlan_get_vid_from_frame (proto, skb) == VLAN_NULL_VID) ? 
        PRIORITY_TAGGED : TAGGED;
    }
  else
    {
      frame_type = UNTAGGED;
    }

  return frame_type;
}

unsigned short
br_vlan_get_nondot1q_ether_type (struct sk_buff *skb)
{
  struct vlan_header *vlan_hdr;
  struct ethhdr *eth = (struct ethhdr *)skb_mac_header (skb);


  if (ntohs(eth->h_proto) == ETH_P_8021Q_STAG)
    {
       vlan_hdr = (struct vlan_header *) skb->data;

       if (ntohs (vlan_hdr->encapsulated_proto) == ETH_P_8021Q_CTAG)
         {
           vlan_hdr = (struct vlan_header *) (skb->data +
                                              sizeof (struct vlan_header));
           return ntohs (vlan_hdr->encapsulated_proto);
         }
       else
         return ntohs (vlan_hdr->encapsulated_proto);
    }
  else if (ntohs(eth->h_proto) == ETH_P_8021Q_CTAG)
    {
      vlan_hdr = (struct vlan_header *) skb->data;
      return ntohs (vlan_hdr->encapsulated_proto);
    }
  else
    return ntohs(eth->h_proto);

  return ntohs(eth->h_proto);
}

/* This function classifies the incoming frame */

vid_t
br_vlan_classify_ingress_frame (unsigned short proto, struct sk_buff *skb)
{
  /* Port based classification */
  /* Get the ingress port through which the frame was received */
  struct apn_bridge_port *port = skb->dev->apn_fwd_port; 

  /* The port is assumed to be a valid ptr as deletion of port from the
     net_device is under bridge lock */
  if (proto == ETH_P_8021Q_CTAG
      && (port->port_type == PRO_NET_PORT
          || port->port_type == CUST_NET_PORT))
    return VLAN_NULL_VID;
  else
    return port->default_pvid;
}

/* This function checks if the given vid is active set of vids */

bool_t
br_vlan_is_vlan_in_active_set (vid_t vid)
{
  /* TODO */
  return BR_TRUE;
}

char *
br_vlan_get_vlan_config_ports (struct apn_bridge *br, enum vlan_type type,
                               vid_t vid)
{
  if (type == CUSTOMER_VLAN)
    return br->static_vlan_reg_table[vid];
  else
    return br->static_svlan_reg_table[vid];
}

/* This function checks if the given port is a member of the given vid */

bool_t
br_vlan_is_port_in_vlans_member_set (struct apn_bridge *br, vid_t vid, int port_no)
{
  struct apn_bridge_port *port=NULL;
  char *config_ports;

  port = br_vlan_get_port_by_no (br, port_no);

  if (! port)
    return BR_FALSE;

  if (port->port_type == PRO_NET_PORT
      || port->port_type == CUST_NET_PORT)
    config_ports = br_vlan_get_vlan_config_ports (br, SERVICE_VLAN,
                                                  vid);
  else
    config_ports = br_vlan_get_vlan_config_ports (br, CUSTOMER_VLAN,
                                                  vid);

  if (!config_ports)
    {
      return BR_FALSE;
    }

  return IS_VLAN_PORT_CONFIGURED (config_ports[port->port_id]);
}

/* This function applies the ingress rules for the incoming frames on a
   vlan-aware bridge */
bool_t
br_vlan_apply_ingress_rules (unsigned short proto,
                             struct sk_buff *skb, vid_t * vid)
{
  /* Apply the Ingress Rules as specified in 802.1Q Section 8.6 */
  struct apn_bridge_port * port = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);
  struct vlan_info_entry **vlan_info_table;
  struct apn_bridge * br = NULL;
  struct vlan_info_entry *entry;
  br_frame_t frame_type;
  unsigned int mtu_val;
  int err;

  if (!port)
    return BR_FALSE;

  br = port->br;

  if (!br)
    return BR_FALSE;

  if (port->port_type == PRO_NET_PORT
      || port->port_type == CUST_NET_PORT)
    vlan_info_table = br->svlan_info_table;
  else
    vlan_info_table = br->vlan_info_table;

  BR_READ_LOCK (&br->lock);

  err = 0; 

  frame_type = br_vlan_get_frame_type (skb, proto);

  switch (frame_type)
    {
      /* 802.1Q: Section 8.6 item a */
    case UNTAGGED:
      {
        /* If control frame */
        if  ((!memcmp (skb->mac_header, group_addr, 5)) ||
             (!memcmp (skb->mac_header, rpvst_group_addr, 6)) ||
             (!memcmp (skb->mac_header, cfm_ccm_addr, 5)))
          {
            /* CFM  - CCM and LTM first 5 bytes are the same */
            *vid = br_vlan_classify_ingress_frame (proto, skb);
            break;
          }

      }
    case PRIORITY_TAGGED:
      {
        BDEBUG ("PRIORITY_TAGGED frame recd\n");

        if (port->acceptable_frame_types & VLAN_ADMIT_TAGGED_ONLY)
          {
            /* Discard the frame */
            BDEBUG ("Discarding untagged frame: Port %d "
                    "admits vlan tagged frames only", port->port_no);
            BR_READ_UNLOCK (&br->lock);
            return BR_FALSE;
          }

        *vid = br_vlan_classify_ingress_frame (proto, skb);
        break;
      }
    case TAGGED:
      {
        BDEBUG ("TAGGED frame recd %d\n", port->port_type);
        if (port->port_type == ACCESS_PORT
              || port->port_sub_type == ACCESS_PORT)
          {
            BR_READ_UNLOCK (&br->lock);
            return BR_FALSE;
          }

        /* Set nh to point past the. */
        skb->network_header += VLAN_HEADER_LEN;

        /* 802.1Q: Section 8.6 item a */
        *vid = br_vlan_get_vid_from_frame (proto, skb);

        BDEBUG ("VID of the Ingress frame %u\n", *vid);

        /* Check if VLAN is configured on switch. */
        entry = vlan_info_table[*vid];

        if (!entry)
          {
            BR_READ_UNLOCK (&br->lock);
            return BR_FALSE;
          }

        if ((*vid > VLAN_MAX_VID) ||
            (!br_vlan_is_vlan_in_active_set (*vid) ))
          {
            /* Discard the frame */
            BDEBUG ("Discarding tagged frame: Port %d "
                    "the VID %d may be above the max VID or"
                    "the VID is not in the active set of VIDs or"
                    "VLAN associated with the VID may be in suspended state",
                    port->port_no, *vid);
            BR_READ_UNLOCK (&br->lock);
            return BR_FALSE;
          }

        break;
      }
    default:
      err = -1;
      break;
    }

  if (err < 0)
    {
      BR_READ_UNLOCK (&br->lock);
      return BR_FALSE;
    }

  if (! vlan_info_table[*vid])
    {
      BR_READ_UNLOCK (&br->lock);
      return BR_FALSE;
    }

  mtu_val = vlan_info_table[*vid]->mtu_val;

  if ( ( mtu_val > 0 ) && ( mtu_val < skb->len ) )
    {
      BDEBUG ( "Frame length (%u) is greater than vlan MTU %u for VLAN %d",
               skb->len, mtu_val, *vid);
      BR_READ_UNLOCK (&br->lock);
      return BR_FALSE;
    }

  if (proto == ETH_P_8021Q_CTAG
      && (port->port_type == PRO_NET_PORT
          || port->port_type == CUST_NET_PORT))
    {
      BDEBUG ( "CVLAN membership for a network port need not be checked");
      read_unlock (&br->lock);
      return BR_TRUE;
    }

  if (port->enable_ingress_filter)
    {
      if (!br_vlan_is_port_in_vlans_member_set (br, *vid, port->port_no))
        {
          BDEBUG ("Discarding tagged/untagged classified frame: Port %d "
                  "enable ingress filter is set and the VID %d"
                  "does not contain the port its its member set",
                  port->port_no, *vid);
          BR_READ_UNLOCK (&br->lock);
          return BR_FALSE;
        }
    }

  BDEBUG ("Accepting the %d frame with VID %d on Port %d\n",
          frame_type, *vid, port->port_no);

  /* Returning true to say that the frame should be further processed by other
     entities */
  BR_READ_UNLOCK (&br->lock);
  return BR_TRUE;
}

/* Push a 802.1q VLAN tag onto a frame */
struct sk_buff *
br_vlan_push_tag_header (struct sk_buff *skb, vid_t vid, unsigned short proto)
{
  struct ethhdr eth_hdr;
  struct vlan_header *vlan_hdr = 0;
  unsigned short vlan_tci;

  BDEBUG("pushing tag header %x \n", vid);

  /* Copy the ethernet header from the skb before the skb_push for VLAN */
  memcpy (&eth_hdr, skb->mac_header, sizeof (struct ethhdr));
  
  /* Make sure that there is enough headroom for the VLAN header */
  if (skb_headroom (skb) < VLAN_ETH_HEADER_LEN)
    {
      struct sk_buff *sk_tmp = skb;
      skb = skb_realloc_headroom (sk_tmp, VLAN_ETH_HEADER_LEN);
      kfree_skb (sk_tmp);
      if (skb == NULL)
        return NULL;
    }

  /* Add VLAN header. */
  vlan_hdr = (struct vlan_header *)skb_push (skb, VLAN_HEADER_LEN);

  /* TODO */
  if (proto == ETH_P_8021Q_CTAG)
    vlan_tci = br_vlan_get_egress_priority(vid);
  else
   {
    vlan_tci = vid;
    vlan_tci |= 0; /* TODO br_vlan_get_egress_priority(dev, skb); */
   }

  vlan_tci = htons (vlan_tci); 
  vlan_hdr->vlan_tci = vlan_tci;
  vlan_hdr->encapsulated_proto = eth_hdr.h_proto;

  /* Create new ethernet header. */
  if ((skb->dev->header_ops != NULL) && (skb->dev->header_ops->create))
    {
      skb->dev->header_ops->create (skb, skb->dev, proto, eth_hdr.h_dest, 
                            eth_hdr.h_source, skb->len + VLAN_HEADER_LEN + ETH_HLEN);
    }
  else
    {
      eth_header (skb, skb->dev, proto, eth_hdr.h_dest,
                  eth_hdr.h_source, skb->len + VLAN_HEADER_LEN + ETH_HLEN);
    }
  
  return skb;
}

static inline struct sk_buff *br_vlan_check_reorder_header(struct sk_buff *skb)
{
  skb = skb_share_check(skb, GFP_ATOMIC);
  if (skb) 
    {
      /* Lifted from Gleb's VLAN code... */
      memmove(skb->data - ETH_HLEN,
              skb->data - VLAN_ETH_HEADER_LEN, 12);
      skb->mac_header += VLAN_HEADER_LEN;
      skb->network_header = skb->data;
    }
  
  return skb;
}

/* Pop a 802.1q VLAN tag off a frame. */
struct sk_buff *
br_vlan_pop_tag_header (struct sk_buff *skb)
{ 
  struct ethhdr eth_hdr;
  struct vlan_header *vlan_hdr = (struct vlan_header*)skb->data;
  unsigned short proto;
  unsigned char *rawp;

  /* Copy the ethernet header from the skb before the skb_pull for VLAN */
  memcpy (eth_hdr.h_source, &skb_src_mac_addr(skb), ETH_ALEN); 
  memcpy (eth_hdr.h_dest, skb->mac_header, ETH_ALEN); 
  eth_hdr.h_proto = vlan_hdr->encapsulated_proto;

  /* pull the vlan information from the skb */
  skb_pull (skb, VLAN_HEADER_LEN);

  skb->protocol = eth_hdr.h_proto;

  if (ntohs (eth_hdr.h_proto) >= 1536)
    {
      proto = ntohs(eth_hdr.h_proto);
    }
  else
    {
      proto = ETH_P_802_3;
      skb->protocol = __constant_htons(ETH_P_802_3);
    }

  skb = br_vlan_check_reorder_header (skb);

  if (!skb)
    return NULL;

  if ((skb->dev->header_ops != NULL) && (skb->dev->header_ops->create))
    {
      /* Create new ethernet header. */
      skb->dev->header_ops->create (skb, skb->dev, proto, eth_hdr.h_dest,
                            eth_hdr.h_source, skb->len + ETH_HLEN);
    }
  else
    {
      eth_header (skb, skb->dev, proto, eth_hdr.h_dest, eth_hdr.h_source,
                  skb->len + ETH_HLEN);
    }

  return skb;
}

/* Swap the tag on a frame that already has a tag. */

void
br_vlan_swap_tag_header (struct sk_buff *skb, vid_t vid)
{ 
  struct vlan_header *vlan_hdr = (struct vlan_header*)skb->data;

  BDEBUG("swapping tag headers \n");

  /* clear the vid in the tag */
  vlan_hdr->vlan_tci &= ~(VLAN_VID_MASK);

  /* set the actual vid */ 
  vlan_hdr->vlan_tci |= htons (vid);
}

/* Returns true if the fid is valid for the bridge */
int
br_vlan_validate_vid (struct apn_bridge *br, vlan_type_t type, unsigned short vid)
{

  if (br->is_vlan_aware && type == CUSTOMER_VLAN)
    return ((vid < VLAN_MAX_VID) && br->vlan_info_table [vid]);
  else if (br->is_vlan_aware && type == SERVICE_VLAN)
    return ((vid < VLAN_MAX_VID) && br->svlan_info_table [vid]);
  else
    return 1;
}

extern void
br_vlan_reset_port_type (struct apn_bridge *br)
{
  struct apn_bridge_port *p;

  BR_READ_LOCK (&br->lock);

  p = br->port_list;

  while (p != NULL)
    {
      p->port_type = p->port_sub_type = ACCESS_PORT;
      p = p->next;
    }

  BR_READ_UNLOCK (&br->lock);

  return;
}
