/*
  Forwarding decision
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
#include <linux/netdevice.h>
#include <linux/inetdevice.h>
#include <linux/skbuff.h>
#include <linux/netfilter_bridge.h>
#include <linux/if_vlan.h>
#include "if_ipifwd.h"
#include "br_types.h"
#include "br_forward.h"
#include "bdebug.h"
#include "br_vlan.h"
#include "br_vlan_api.h"
#include "br_pvlan_api.h"
#include "br_pro_vlan.h"

/* Determine if a frame should be sent on a port - if so, send it. */

static __inline__ int 
should_deliver (struct apn_bridge_port *port, struct sk_buff *skb, 
                unsigned short instance, vid_t cvid  )
{
  int ret ;
  unsigned char forward;
 
  if (port->port_type == CUST_EDGE_PORT)
    forward = (br_cvlan_state_flags_get (port, cvid)) & APNFLAG_FORWARD;
  else 
    forward = port->state_flags[instance] & APNFLAG_FORWARD ;

  /* reverse logic */
  ret = ((skb->dev != port->dev) 
         && (forward != 0) 
         && ((port->dev->flags & IFF_UP) != 0)) ? 1 : 0;

  

  BDEBUG(" In should deliver for port %d  instance  %d \n",
         port->port_no, instance);
  if (ret == 0)
    {
      BDEBUG ("Cannot forward as : %s%s%s \n",
              (skb->dev == port->dev) ? "Device same as i/p dev " : "" ,
              (forward == 0) ? "State not fwding for inst " : "",
              ((port->dev->flags & IFF_UP) == 0 ) ? "Device not up " : "" );
    }
  else
    {
      BDEBUG ("Can forward\n");
    }

  return ret;
}

static int 
br_transmit (struct apn_bridge_port *to, struct sk_buff *skb)
{
  struct net_device *indev;
  
  indev = skb->dev;
  skb->dev = to->dev;
  skb_push (skb, ETH_HLEN);
  dev_queue_xmit (skb);
  
  BDEBUG("Transmitted frame on a transparent bridge for port(%u)\n",
         to->port_no); 
  return 0;
}

static __inline__ int
br_vlan_transmit_thru_net_port (struct sk_buff *skb, vid_t svid,
                                 bool_t is_egress_tagged)
{
  /* Check if the frame is a tagged or untagged or priority tagged frame */
  struct apn_bridge_port *port;
  
  br_frame_t frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_STAG);

  switch (frame_type)
    {
      case UNTAGGED:
        {
          BDEBUG("Push vlan tag header for untagged tagged frame on "
                 "network port\n");

          if (is_egress_tagged)
            skb = br_vlan_push_tag_header (skb, svid, ETH_P_8021Q_STAG);

          break;
        }
      case PRIORITY_TAGGED:
        {
          BDEBUG("Swap vlan tag header for priority tagged frame on "
                 "network port\n");

          skb = br_vlan_pop_tag_header (skb);
          skb_pull (skb, ETH_HLEN);

          if (is_egress_tagged)
            skb = br_vlan_push_tag_header (skb, svid, ETH_P_8021Q_STAG);
          else
            skb_push (skb, ETH_HLEN);

          break;
        }
      case TAGGED:
        {
          BDEBUG("No-op for tagged frame on trunk port\n");

          if (is_egress_tagged)
            skb_push (skb, ETH_HLEN);
          else
            skb = br_vlan_pop_tag_header (skb);

          break;
        }
      default:
        BERROR ("Unknown frame type encountered?\n");
        /* Error */
        break;
    }

  if (skb)
    {
      port = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);

      br_vlan_translate_egress_vid (port, skb);

      BDEBUG("Transmitting frame_type(%u) through network port(%u)\n",
             frame_type, port->port_no);
      /* Transmit the frame */
      return dev_queue_xmit (skb);
    }

  return -1;
}

static __inline__ int
br_vlan_transmit_thru_ce_port (struct sk_buff *skb)
{
  /* Check if the frame is a tagged or untagged or priority tagged frame */
  vid_t cvid;
  vid_t svid;
  u_int8_t s_tagged = UNTAGGED;
  struct apn_bridge *br;
  char *config_ports = 0;
  struct apn_bridge_port *port;
  bool_t is_egress_tagged = BR_FALSE;
  struct vlan_header *vlan_hdr;
  struct br_vlan_pro_edge_port *pep = NULL;
  br_frame_t frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_STAG);

  port = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);

  if (port == NULL)
    return -1;

  br = port->br;

  if (br == NULL)
    return -1;

  switch (frame_type)
    {
      case UNTAGGED:
      case PRIORITY_TAGGED:
        BDEBUG("Error condition. The frame should always have S Tag");
        return -1;
        break;
      case TAGGED:
        BDEBUG("Pop vlan tag header for priority tagged/tagged frame on "
               "CUSTOMER EDGE PORT\n");
        /* This svid will be used in case the frame was on maintenance vid and 
         * to build the ctag in it. */
        s_tagged = TAGGED;
        svid = br_vlan_get_vid_from_frame (ETH_P_8021Q_STAG, skb);

        /* pop the tag header from the frame and forward the frame */
        skb = br_vlan_pop_tag_header (skb);
        
        break;
      default:
        /* Error */
        break;
    }

  frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_CTAG);

  switch (frame_type)
    {
      case UNTAGGED:
      case PRIORITY_TAGGED:
        break;
      case TAGGED:

        vlan_hdr = (struct vlan_header *) (skb->data + ETH_HLEN);

        cvid = ntohs(vlan_hdr->vlan_tci) & VLAN_VID_MASK;

        config_ports = br_vlan_get_vlan_config_ports
                                                 ( br, CUSTOMER_VLAN, cvid );

        if (config_ports == NULL)
          {
            BDEBUG("vid not confiugred on this port\n");
            return -1;

          }
        /* Check if the egress side the frame should be tagged */
        is_egress_tagged =
                IS_VLAN_EGRESS_TAGGED ( config_ports[port->port_id] );


        BDEBUG("Pop customer vlan tag header for priority tagged/tagged "
               "frame on CUSTOMER EDGE PORT\n");

        /* pop the tag header from the frame and forward the frame */
        if (is_egress_tagged == BR_FALSE)
          {
            skb_pull (skb, ETH_HLEN);
            skb = br_vlan_pop_tag_header (skb);
          }

        break;
      default:
        /* Error */
        break;
    }

  /* If the frame does not contain a ctag, then it wont be evaluated to be 
   * egress tagged or not. 
   * Frames on maintenance vlan will have only stag and no ctag. 
   * So, even those frames should be considered for egress tagged.
   * For control protocol, egress from CEP (eg. CFM) is taken care by PM 
   * for maintenance vlan. The following addition of ctag in the outgoing
   * frame will come into picture for data frames */

  if (skb)
    {
      if (frame_type == TAGGED
          && (is_egress_tagged & VLAN_EGRESS_TAGGED))
        br_vlan_translate_egress_vid (port, skb);

      if ((s_tagged == TAGGED) && 
          (frame_type == UNTAGGED))
        {
          /* Frame on maintenance vlan being sent out of CEP.
           * No ctag availble only stag was present. So, adding ctag */
          is_egress_tagged = BR_FALSE;
 
          /* Find the PEP to determine the ctag which has to be encoded */
          pep = br_vlan_pro_edge_port_lookup (port, svid);

          if (pep != NULL)
            {
              /* The outgoing cvid would be the default vid of the PEP */
              config_ports = br_vlan_get_vlan_config_ports
                ( br, CUSTOMER_VLAN, pep->pvid);

              if (config_ports != NULL)
                {
                  /* Check if the egress side the frame should be tagged */
                  is_egress_tagged =
                    IS_VLAN_EGRESS_TAGGED ( config_ports[port->port_id] );
                }

              /* Outgoing frames to be tagged */
              if (is_egress_tagged != BR_FALSE)
                {
                  /* Add the maintenance vid on this outgoing frame */
                  
                  /* The stag was POPed, for that this skb_pull is present */
                  skb_pull (skb, ETH_HLEN);

                  /* Putting ctag in the frame */
                  skb = br_vlan_push_tag_header (skb, pep->pvid, ETH_P_8021Q_CTAG);
                  skb->mac_header = skb->data;

                  /* In case of translation */
                  if (is_egress_tagged & VLAN_EGRESS_TAGGED)
                    br_vlan_translate_egress_vid (port, skb);

                }/* if (is_egress_tagged != BR_FALSE) */
            }
        }/* if ((s_tagged == TAGGED) && 
          (frame_type == UNTAGGED)) */

      BDEBUG("Transmitting frame_type(%u) through access port(%u)\n",
              frame_type, port->port_no);

      /* Transmit the frame */
      return dev_queue_xmit (skb);
    }

  return -1;
}

static __inline__ int 
br_vlan_transmit_thru_access_port (struct sk_buff *skb)
{
  struct apn_bridge_port *port;
  /* Check if the frame is a tagged or untagged or priority tagged frame */
  br_frame_t frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_CTAG);

  switch (frame_type)
    {
    case UNTAGGED:
      BDEBUG("No-op for untagged frame on access port\n");
      skb_push (skb, ETH_HLEN);
      break;
    case PRIORITY_TAGGED:
    case TAGGED:
      BDEBUG("Pop vlan tag header for priority tagged/tagged frame on "
             "access port\n");
      /* pop the tag header from the frame and forward the frame */
      skb = br_vlan_pop_tag_header (skb);
      break;
    default:
      /* Error */
      break;
    }
  
  if (skb)
    {
      port = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);
      BDEBUG("Transmitting frame_type(%u) through access port(%u)\n", 
             frame_type, port->port_no);
     
      /* Transmit the frame */
      return dev_queue_xmit (skb);
    }
  
  return -1;
}

static __inline__ int
br_vlan_transmit_thru_hybrid_port (struct sk_buff *skb, vid_t vid, 
                                   bool_t is_egress_tagged)
{
  struct apn_bridge_port *port;
  /* Check if the frame is a tagged or untagged or priority tagged frame */
  br_frame_t frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_CTAG);

  switch (frame_type)
    {
    case UNTAGGED:
      {
        if (is_egress_tagged)
          {
            BDEBUG("Push vlan tag header for untagged tagged frame on "
                   "hybrid port\n");
            skb = br_vlan_push_tag_header (skb, vid, ETH_P_8021Q_CTAG);
          }
        else 
          {
            skb_push (skb, ETH_HLEN);
          }
        break;
      }
    case PRIORITY_TAGGED:
      {
        if (is_egress_tagged)
          {
            BDEBUG("Swap vlan tag header for priority tagged frame on "
                   "hybrid port\n");
            skb = br_vlan_pop_tag_header (skb);
            skb_pull (skb, ETH_HLEN);
            skb = br_vlan_push_tag_header (skb, vid, ETH_P_8021Q_CTAG);
          }
        else 
          {
            BDEBUG("Pop vlan tag header for priority tagged frame on "
                   "hybrid port\n");
            skb = br_vlan_pop_tag_header (skb);
          }
        break;
      }
    case TAGGED:
      {
        /* Check the portmap associated with the vid to see if the
           port should forward a frame tagged or untagged */
        if (!is_egress_tagged) 
          {
            BDEBUG("Pop vlan tag header for tagged frame on hybrid port\n");
            skb = br_vlan_pop_tag_header (skb); 
          }
        else
          {
            BDEBUG("No-op for tagged frame on hybrid port\n");
            skb_push (skb, ETH_HLEN);
          } 
        break;
      }
    default:
      /* Error */
      break;
    }
  
  if (skb)
    {
      port = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);
      BDEBUG("Transmitting frame_type(%u) through hybrid port(%u)\n", 
             frame_type, port->port_no);
      
      /* Transmit the frame */
      return dev_queue_xmit (skb);
    }
  
  return -1;
}

static __inline__ int
br_vlan_transmit_thru_trunk_port (struct sk_buff *skb, vid_t vid)
{
  struct apn_bridge_port *port;
  /* Check if the frame is a tagged or untagged or priority tagged frame */
  br_frame_t frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_CTAG);
 
  switch (frame_type)
    {
    case UNTAGGED:
      {
        BDEBUG("Push vlan tag header for untagged tagged frame on "
               "trunk port\n");
        skb = br_vlan_push_tag_header (skb, vid, ETH_P_8021Q_CTAG);
        break;
      }
    case PRIORITY_TAGGED:
      {
        BDEBUG("Swap vlan tag header for priority tagged frame on "
               "trunk port\n");

        skb = br_vlan_pop_tag_header (skb);
        skb_pull (skb, ETH_HLEN);
        skb = br_vlan_push_tag_header (skb, vid, ETH_P_8021Q_CTAG);
        break;
      }
    case TAGGED:
      {
        BDEBUG("No-op for tagged frame on trunk port\n");
        skb_push (skb, ETH_HLEN);
        break;
      }
    default:
      BERROR ("Unknown frame type encountered?\n");
      /* Error */
      break;
    }

  if (skb)
    {
      port = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);
      BDEBUG("Transmitting frame_type(%u) through trunk port(%u)\n", 
             frame_type, port->port_no);
      /* Transmit the frame */
      return dev_queue_xmit (skb);
    }
  
  return -1;
}

static void
br_vlan_transmit (struct apn_bridge_port *to, vid_t vid, struct sk_buff *skb,
                  bool_t is_egress_tagged)
{
  struct net_device *indev;
  struct apn_bridge_port *port;
  int ret = 0;

  indev = skb->dev;
  skb->dev = to->dev;
  port = (struct apn_bridge_port *) (skb->dev->apn_fwd_port);

  if (!port)
    return;

  switch (port->port_type)
    {
    case ACCESS_PORT:
      ret = br_vlan_transmit_thru_access_port (skb);
      break;
    case HYBRID_PORT:
      ret = br_vlan_transmit_thru_hybrid_port (skb, vid, is_egress_tagged);
      break;
    case TRUNK_PORT:
      ret = br_vlan_transmit_thru_trunk_port (skb, vid);
      break;
    case CUST_EDGE_PORT:
      ret = br_vlan_transmit_thru_ce_port (skb);
      break;
    case PRO_NET_PORT:
    case CUST_NET_PORT:
      ret = br_vlan_transmit_thru_net_port (skb, vid, is_egress_tagged);
      break;
    default:
      /* Error */
      break;
    }

  /* We have to do a hack here figure out if the indev is actually a VI.
   * This is to update the frame txmit stats.
   */
  if ((ret < 0) && indev && (indev->apn_fwd_port == NULL))
    {
      struct net_device_stats *stats = indev->get_stats (indev);
      if (stats)
        {
          stats->tx_dropped++;
          stats->tx_errors++;
        }
    }
}

static void
br_pvlan_transmit (struct apn_bridge *br, struct sk_buff *skb, 
                   unsigned short instance, vid_t vid,
                   char *pvlan_config_ports, vid_t *port_default_vid)
{
  struct apn_bridge_port *rcvd_port
    = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);  
  struct vlan_info_entry *vlan = br->vlan_info_table[vid];
  struct apn_bridge_port *port = br->port_list;
  struct vlan_info_entry *secondary_vlan = NULL;
  vid_t secondary_vid;
  
  if (!vlan)
    return;

  switch (vlan->pvlan_type)
    {
    case PVLAN_ISOLATED:
      while (port)
        {
          if (IS_VLAN_PORT_CONFIGURED (pvlan_config_ports[port->port_id]) &&
              should_deliver (port, skb, instance, VLAN_NULL_VID))
            {
              BDEBUG ("VLAN configured on port id %d \n", port->port_id);
              /* Clone the buffer and forward to the indicated port */
              struct sk_buff *skb2 = NULL;

              if ((skb2 = skb_copy (skb, GFP_ATOMIC)) == NULL)
                {
                  br->statistics.tx_dropped++;
                  kfree_skb (skb);
                  BDEBUG("Memory allocation failure while skb_clone\n");
                  /* RELEASE READ LOCK FOR STATIC_VLAN_REG_TABLE */
                  return;
                }
              if (rcvd_port->pvlan_port_mode == PVLAN_PORT_MODE_INVALID)
                {
                  if ((port->pvlan_port_mode == PVLAN_PORT_MODE_INVALID)
                      ||(port->pvlan_port_mode == PVLAN_PORT_MODE_PROMISCUOUS))
                    {
                      br_vlan_transmit (port, port_default_vid[port->port_id],
                                        skb2,
                                        IS_VLAN_EGRESS_TAGGED
                                        (pvlan_config_ports[port->port_id]));
                    }
                }
              else
                {
                  br_vlan_transmit (port, port_default_vid[port->port_id],
                                    skb2,
                                    IS_VLAN_EGRESS_TAGGED
                                    (pvlan_config_ports[port->port_id]));
                }
            }
          port = port->next;
        }
                    
      break;
    case PVLAN_COMMUNITY:
      while (port)
        {
          if (IS_VLAN_PORT_CONFIGURED (pvlan_config_ports[port->port_id]) &&
              should_deliver (port, skb, instance, VLAN_NULL_VID))
            {
              BDEBUG ("VLAN configured on port id %d \n", port->port_id);
              /* Clone the buffer and forward to the indicated port */
              struct sk_buff *skb2 = NULL;

              if ((skb2 = skb_copy (skb, GFP_ATOMIC)) == NULL)
                {
                  br->statistics.tx_dropped++;
                  kfree_skb (skb);
                  BDEBUG("Memory allocation failure while skb_clone\n");
                  /* RELEASE READ LOCK FOR STATIC_VLAN_REG_TABLE */
                  return;
                }
              br_vlan_transmit (port, port_default_vid[port->port_id],
                                skb2,
                                IS_VLAN_EGRESS_TAGGED
                                (pvlan_config_ports[port->port_id]));
            }
          port = port->next;
        }

      break;
    case PVLAN_PRIMARY:
      while (port)
        {
          if (IS_VLAN_PORT_CONFIGURED (pvlan_config_ports[port->port_id]) &&
              should_deliver (port, skb, instance, VLAN_NULL_VID))
            {
              BDEBUG ("VLAN configured on port id %d \n", port->port_id);
              /* Clone the buffer and forward to the indicated port */
              struct sk_buff *skb2 = NULL;

              if ((skb2 = skb_copy (skb, GFP_ATOMIC)) == NULL)
                {
                  br->statistics.tx_dropped++;
                  kfree_skb (skb);
                  BDEBUG("Memory allocation failure while skb_clone\n");
                  /* RELEASE READ LOCK FOR STATIC_VLAN_REG_TABLE */
                  return;
                }
              if (rcvd_port->pvlan_port_mode == PVLAN_PORT_MODE_INVALID)
                {
                  if (port->pvlan_port_mode == PVLAN_PORT_MODE_HOST)
                    {
                      secondary_vid = port_default_vid[port->port_id];
                      secondary_vlan = br->vlan_info_table[secondary_vid];
                      if (!secondary_vlan)
                        break;
                      if (secondary_vlan->pvlan_type == PVLAN_COMMUNITY)
                        br_vlan_transmit (port, port_default_vid[port->port_id],
                                          skb2,
                                          IS_VLAN_EGRESS_TAGGED
                                          (pvlan_config_ports[port->port_id]));
                    }
                  else
                    {
                      br_vlan_transmit (port, port_default_vid[port->port_id],
                                        skb2,
                                        IS_VLAN_EGRESS_TAGGED
                                        (pvlan_config_ports[port->port_id]));
                    }
                }
              else
                {
                  br_vlan_transmit (port, port_default_vid[port->port_id],
                                    skb2,
                                    IS_VLAN_EGRESS_TAGGED
                                    (pvlan_config_ports[port->port_id]));
                }
            }

          port = port->next;
        }

      break;
    default:
      break;
    }

  kfree (skb);
}

/* Apply the egress rules for the VLAN. */

static __inline__ bool_t
br_vlan_apply_egress_rules (struct apn_bridge *br, struct sk_buff *skb, 
                            vid_t vid, int port_no)
{
  unsigned int mtu_val ;
  struct apn_bridge_port *port;
  struct vlan_info_entry *vinfo;

  if (!br)
    return BR_FALSE;

  port = br_vlan_get_port_by_no (br, port_no);

  if (!port)
    return 0;

  if (port->port_type == PRO_NET_PORT
      || port->port_type == CUST_NET_PORT)
    vinfo = br->svlan_info_table[vid];
  else
    vinfo = br->vlan_info_table[vid];

  if ( vinfo == NULL )
    return BR_FALSE;

  mtu_val = vinfo->mtu_val;

  /* 802.1Q: Section 8.8 item a */
  BDEBUG("applying egress rules vid(%u), port_no(%u)\n", vid, port_no);

  if (br_vlan_is_port_in_vlans_member_set (br, vid, port_no) )
    {
      if ( ( mtu_val > 0 ) && ( mtu_val < skb->len ))
        {
          BDEBUG ( "Frame length(%u) is greater than vlan MTU(%u) for VLAN  %d \n",
                   skb->len, mtu_val, vid );
          return 0;
        }

      return 1;
    }
  else
    {
      return 0;
    }

  /* 802.1Q: Section 8.8 item b, c, d Not implemented as the stp socket
     support is not available */
}

/* Forward a frame - called under bridge lock */
void 
br_forward (struct apn_bridge_port *to, vid_t vid, struct sk_buff *skb,
            bool_t is_egress_tagged)
{
  struct sk_buff *skb2;
  unsigned short instance;
  struct vlan_info_entry *vinfo;


  /* get the instance of the port asso with the vid */
  if (to->br->is_vlan_aware)
    {
      if (to->port_type == PRO_NET_PORT
          || to->port_type == CUST_NET_PORT)
        vinfo = to->br->svlan_info_table[vid];
      else
        vinfo = to->br->vlan_info_table[vid];

      if (vinfo == NULL)
        return;

      instance = vinfo->instance;
    }
  else
    instance = BR_INSTANCE_COMMON;

  skb2 = skb_copy (skb, GFP_ATOMIC);

  if (skb2 == NULL)
    {
      kfree_skb (skb);
      return;
    }

  kfree_skb (skb);
  skb = skb2;

  if (should_deliver (to, skb, instance, vid))
    {
      BDEBUG("should deliver is true for port %d - scheduling\n", to->port_no);

      if (to->br->is_vlan_aware) /* vlan-aware bridge */
        {
          if (br_vlan_apply_egress_rules (to->br, skb, vid, to->port_no))
            {
              br_vlan_transmit (to, vid, skb, is_egress_tagged);
              return;
            }
        }
      else /* tranparent bridge */
        {
          br_transmit (to, skb);
        }
    } 
  else 
    {
      BDEBUG("should deliver is false for port %d\n", to->port_no);
      kfree_skb (skb);
    }

}


char *
br_pvlan_get_configure_ports (struct apn_bridge *br, vid_t vid,
                              vid_t *default_vid)
{
  struct apn_bridge_port *p;
  char *local_config_ports = kmalloc (BR_MAX_PORTS * sizeof (char), GFP_ATOMIC);
  struct vlan_info_entry *vlan, *port_vlan;
  vid_t primary_vid, secondary_vid;
  int port_id;
  char *config_ports = 0;
  bool_t default_vid_created = BR_FALSE;
  char *trunk_config_ports = kmalloc (BR_MAX_PORTS * sizeof (char), GFP_ATOMIC);
  
  if (!br->is_vlan_aware)
    {
      BDEBUG ("Bridge is not vlan aware \n");
      kfree (trunk_config_ports);
      kfree (local_config_ports);
      return NULL;
    }

  if (!default_vid)
    {
      BDEBUG (" default_vid is null \n");
      default_vid = kmalloc (BR_MAX_PORTS * sizeof (vid_t), GFP_ATOMIC);
      default_vid_created = BR_TRUE;
    }

  for (port_id = 0; port_id < BR_MAX_PORTS; port_id++)
    default_vid [port_id] = 0;

  /* get the trunk and hybrid ports configuration */
  config_ports = br_vlan_get_vlan_config_ports (br, CUSTOMER_VLAN,
                                                vid);
  if (config_ports)
    {
      memcpy (trunk_config_ports, config_ports, BR_MAX_PORTS);
      for (port_id = 0; port_id < BR_MAX_PORTS; port_id++)
        {
          if (!(trunk_config_ports[port_id] & VLAN_PORT_CONFIGURED))
            continue;

          BR_READ_UNLOCK (&br->lock);
          if ((p = br_vlan_get_port_by_id (br, port_id)) != NULL)
            {
              /* See whether the port is egress tagged or not (ie, trunk port 
               * or interlink switchport) 
               * if yes, port configuration is retained else resetted 
               */
              BDEBUG (" port_id = %d egress tagged = %d  vid = %d \n",
                      p->port_id,
                      IS_VLAN_EGRESS_TAGGED (trunk_config_ports[p->port_id]),
                      vid);

              if (p->pvlan_port_mode != PVLAN_PORT_MODE_INVALID)
                trunk_config_ports [p->port_id] &= 0;
              else
                default_vid [p->port_id] = vid;       
            }
          BR_READ_LOCK (&br->lock);
          BDEBUG (" trunk_config_ports[%d] = %d, default_vid = %d \n ",
                  port_id, trunk_config_ports [port_id],default_vid [port_id]);
        }
    }

  vlan = br->vlan_info_table[vid];
  if (!vlan)
    {
      if (default_vid_created)
        kfree (default_vid);
      kfree (trunk_config_ports);
      return NULL;
    }
  
  if (vlan->pvlan_configured)
    {
      BDEBUG (" pvlan configured on %d \n", vid);
      BDEBUG (" vlan type %d \n", vlan->pvlan_type);
      switch (vlan->pvlan_type)
        {
        case PVLAN_ISOLATED:
          BDEBUG (" Isolated \n");
          primary_vid = vlan->pvlan_info.vid.primary_vid;

          /* Get the port list associated with the vid */
          config_ports = br_vlan_get_vlan_config_ports (br, CUSTOMER_VLAN,
                                                        primary_vid);

          memcpy (local_config_ports, config_ports, BR_MAX_PORTS);
          for (port_id = 0; port_id <BR_MAX_PORTS; port_id++)
            {
              if (local_config_ports[port_id] & VLAN_PORT_CONFIGURED)
                default_vid [port_id] = primary_vid;
            }
          break;

        case PVLAN_COMMUNITY:
          /* Get the port list associated with the vid */
          config_ports = br_vlan_get_vlan_config_ports (br, CUSTOMER_VLAN, vid);
          memcpy (local_config_ports, config_ports, BR_MAX_PORTS);
          for (port_id = 0; port_id <BR_MAX_PORTS; port_id++)
            {
              if (!(local_config_ports[port_id] & VLAN_PORT_CONFIGURED))
                continue;
              default_vid[port_id]=vid;
            }
          primary_vid = vlan->pvlan_info.vid.primary_vid;

          config_ports = br_vlan_get_vlan_config_ports (br, CUSTOMER_VLAN,
                                                        primary_vid);
                  
          for (port_id = 0; port_id <BR_MAX_PORTS; port_id++)
            {
              if (!(config_ports[port_id] & VLAN_PORT_CONFIGURED))
                continue;
              if (!(local_config_ports [port_id] & VLAN_PORT_CONFIGURED))
                {
                  local_config_ports [port_id] |= config_ports [port_id];
                  default_vid[port_id] = primary_vid;
                }
            }   
          break;
                
        case PVLAN_PRIMARY:
          config_ports = br_vlan_get_vlan_config_ports (br, CUSTOMER_VLAN,
                                                        vid);
          memcpy (local_config_ports, config_ports, BR_MAX_PORTS);
          for (port_id = 0; port_id <BR_MAX_PORTS; port_id++)
            {
              if (!(local_config_ports[port_id] & VLAN_PORT_CONFIGURED))
                continue;
              default_vid[port_id]=vid;
            }
          for (secondary_vid = 2; secondary_vid < VLAN_MAX_VID;
               secondary_vid++)
            {
              port_vlan = br->vlan_info_table[secondary_vid];
              if (!port_vlan)
                continue;
              if ((BR_SERV_REQ_VLAN_IS_SET
                   (vlan->pvlan_info.secondary_vlan_bmp, secondary_vid)))
                {

                  config_ports = br_vlan_get_vlan_config_ports (br, CUSTOMER_VLAN,
                                                                secondary_vid);
                  if (!config_ports)
                    break;
                  for (port_id = 0; port_id <BR_MAX_PORTS; port_id++)
                    {
                      if (!(config_ports[port_id] & VLAN_PORT_CONFIGURED))
                        continue;
                      if (!(local_config_ports [port_id] & VLAN_PORT_CONFIGURED))
                        {
                          local_config_ports [port_id] |= config_ports [port_id];
                          default_vid[port_id] = secondary_vid;
                        }
                    }  /* end of port_id loop */
                }

            } /* End of vlan loop */
        } /* end of switch */
    } /* pvlan_configured */ 

  if (local_config_ports && trunk_config_ports)
    {
      /* copy the trunk and hybride ports configurationto local_config_ports*/ 
      for (port_id = 0; port_id < BR_MAX_PORTS; port_id++)
        {
          if (!(trunk_config_ports[port_id] & VLAN_PORT_CONFIGURED))
            {
              BDEBUG ("config_ports[%d] = %c default_vid = %d \n", port_id,
                      local_config_ports[port_id], default_vid [port_id]);
              continue;
            }
         else
           {
             if (!(local_config_ports [port_id] & VLAN_PORT_CONFIGURED))
               {
                 local_config_ports [port_id] |= trunk_config_ports [port_id];
                 default_vid[port_id] = vid;
               }
           }
        }
    }
  kfree (trunk_config_ports);
  return local_config_ports;
}

void
br_provider_vlan_flood (struct apn_bridge *br, struct sk_buff *skb,
                        vid_t cvid, vid_t svid, int clone)
{
  char cust_edge_br;
  char* config_ports = 0;
  struct apn_bridge_port *port = br->port_list;
  unsigned char instance;
  struct apn_bridge_port *rcvd_port = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);

  if (port == NULL)
    return;

  if (br->svlan_info_table[svid])
    {
      instance =  br->svlan_info_table[svid]->instance;
    }
  else
    return ;

  if (clone)
    {
      struct sk_buff *skb2;

      if ((skb2 = skb_clone (skb, GFP_ATOMIC)) == NULL)
        {
          br->statistics.tx_dropped++;
          return;
        }

      kfree_skb(skb);
      skb = skb2;
    }

  cust_edge_br = (br->num_ce_ports || br->num_cn_ports ||
                  br->num_cnp_ports || br->num_pip_ports);

  while (port)
    {
      if (cvid == VLAN_NULL_VID)
        cvid = port->default_pvid;

      if (port->port_type == CUST_NET_PORT
          || port->port_type == PRO_NET_PORT)
        config_ports = br_vlan_get_vlan_config_ports
                                        ( br, SERVICE_VLAN, svid );
      else
        config_ports = br_vlan_get_vlan_config_ports
                                        ( br, CUSTOMER_VLAN, cvid );

      if (config_ports == NULL)
        {
          port = port->next;
          continue;
        }

      /* If the SVLAN is not configured on the port, do not forward */

      if (port->port_type == CUST_EDGE_PORT
          && (br_vlan_pro_edge_port_lookup (port, svid) == NULL))
        {
          port = port->next;
          continue;
        }

      if (IS_VLAN_PORT_CONFIGURED (config_ports[port->port_id]) &&
          should_deliver (port, skb, instance, cvid))
        {
          BDEBUG ("VLAN configured on port id %d \n", port->port_id);

          /* Clone the buffer and forward to the indicated port */

          struct sk_buff *skb2 = NULL;

          if ((skb2 = skb_copy (skb, GFP_ATOMIC)) == NULL)
            {
              br->statistics.tx_dropped++;
              kfree_skb (skb);
              BDEBUG("Memory allocation failure while skb_clone\n");
              /* RELEASE READ LOCK FOR STATIC_VLAN_REG_TABLE */
              return;
            }

          /* Egress rules are not applied on the frames that are flooded as
             the ports through which flooding is done is already in the
             member set of the vid specified */
              
          br_vlan_transmit (port, svid,
                            skb2,
                            IS_VLAN_EGRESS_TAGGED (config_ports[port->port_id]));
        }
      else
        {
           BDEBUG ("VLAN not configured on port id %d \n", port->port_id);
        }

        port= port->next;
    }

  /* free skb as only skb2 is freed in dev_queue_xmit */
  kfree_skb (skb);
}

/* Flood a frame over the specified vlan. */

static void
br_vlan_flood (struct apn_bridge *br, struct sk_buff *skb, vid_t vid, int clone)
{
  char* config_ports = 0;
  char *pvlan_config_ports = 0;
  vid_t port_default_vid[BR_MAX_PORTS];
  struct apn_bridge_port *port = br->port_list;
  struct vlan_info_entry *vlan = br->vlan_info_table[vid];
  unsigned short instance =  br->vlan_info_table[vid]->instance ;
  struct apn_bridge_port *rcvd_port = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);

  if (!port)
    {
      return;
    }
 
  if (clone)
    {
      struct sk_buff *skb2; 

      if ((skb2 = skb_clone (skb, GFP_ATOMIC)) == NULL)
        {
          br->statistics.tx_dropped++;
          return;
        }
      kfree_skb(skb);
      skb = skb2;
    }

  if (!vlan)
    return;

  if ((vlan) && (vlan->pvlan_configured))
    {
      pvlan_config_ports =  br_pvlan_get_configure_ports (br, vid,
                                                          port_default_vid);
      if (!pvlan_config_ports)
        { 
          BDEBUG ("ports are not configured for the given vid(%u)\n", vid);
          return;
        }

      br_pvlan_transmit (br, skb, instance, vid, pvlan_config_ports, port_default_vid);
      if (pvlan_config_ports)
        kfree (pvlan_config_ports);
      return;
    }
  else
    {  
      /* Get the port list associated with the vid */
      config_ports = br_vlan_get_vlan_config_ports (br, CUSTOMER_VLAN, vid);
    }

  if (!config_ports)
    {
      BDEBUG("ports are not configured for the given vid(%u)\n", vid);
      return;
    }
  
  /* TAKE READ LOCK FOR STATIC_VLAN_REG_TABLE */
  while (port)
    {
      if (IS_VLAN_PORT_CONFIGURED (config_ports[port->port_id]) &&
          should_deliver (port, skb, instance, VLAN_NULL_VID))
        {
          BDEBUG ("VLAN configured on port id %d \n", port->port_id);
          /* Clone the buffer and forward to the indicated port */
          struct sk_buff *skb2 = NULL; 
            
          if ((skb2 = skb_copy (skb, GFP_ATOMIC)) == NULL)
            {
              br->statistics.tx_dropped++;
              kfree_skb (skb);
              BDEBUG("Memory allocation failure while skb_clone\n");
              /* RELEASE READ LOCK FOR STATIC_VLAN_REG_TABLE */
              return;
            }
              
          /* Egress rules are not applied on the frames that are flooded as
             the ports through which flooding is done is already in the
             member set of the vid specified */
          br_vlan_transmit (port, vid,
                            skb2, IS_VLAN_EGRESS_TAGGED (config_ports[port->port_id]));
        }
      else
        {
          BDEBUG ("VLAN not configured on port id %d \n", port->port_id);
        }

      port= port->next;
    }

  /* free skb as only skb2 is freed in dev_queue_xmit */
  kfree_skb (skb);
}

/* Flood a frame - no vlan. Called under bridge lock */

void
br_flood (struct apn_bridge *br, struct sk_buff *skb, int clone)
{
  struct apn_bridge_port *port = br->port_list;

  if (!port)
    {
      return;
    }

  if (clone)
    {
      struct sk_buff *skb2;

      if ((skb2 = skb_clone (skb, GFP_ATOMIC)) == NULL)
        {
          br->statistics.tx_dropped++;
          return;
        }

      skb = skb2;
    }
  /* Always called for vlan unaware bridge so instance is 0 */
  while (port)
    {
      if (should_deliver (port, skb, BR_INSTANCE_COMMON, VLAN_NULL_VID))
        {
          struct sk_buff *skb2 = NULL;

          if ((skb2 = skb_copy (skb, GFP_ATOMIC)) == NULL)
            {
              br->statistics.tx_dropped++;
              kfree_skb (skb);
              return;
            }

          br_transmit (port, skb2);
        }

      port = port->next;
    }

  /* free skb as only skb2 is freed in dev_queue_xmit */
  kfree_skb (skb);
}

/* This function forwards a frame. It called under bridge lock. */
void
br_flood_forward (struct apn_bridge *br, struct sk_buff *skb,
                  vid_t cvid, vid_t svid, int clone)
{
  if (br->type == BR_TYPE_PROVIDER_RSTP
      || br->type == BR_TYPE_PROVIDER_MSTP
      || br->type == BR_TYPE_BACKBONE_MSTP
      || br->type == BR_TYPE_BACKBONE_RSTP)
    {
      br_provider_vlan_flood (br, skb, cvid, svid, clone);
    }
  else if (br->is_vlan_aware)
    {
      br_vlan_flood (br, skb, cvid, clone);
    }
  else
    {
      br_flood (br, skb, clone);
    }
}

