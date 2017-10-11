/*
  Handle incoming frames
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
#include <linux/etherdevice.h>
#include <linux/netfilter_bridge.h>
#include <linux/in.h>
#include <net/ip.h>
#include <net/protocol.h>
#include <linux/skbuff.h>
#include <config.h> 
#include "if_ipifwd.h"
#include "br_types.h"
#include "br_input.h"
#include "br_fdb.h"
#include "br_forward.h"
#include "bdebug.h"
#include "br_llc.h"
#include "br_vlan.h"
#include "br_pro_vlan.h"
#include "br_pvlan_api.h"
#ifdef  HAVE_CFM
#include "af_cfm.h"
#endif /* HAVE_CFM */

/* Define the group address */
const unsigned char gmrp_group_addr[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x20 };
const unsigned char gvrp_group_addr[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x21 };
const unsigned char group_addr[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x00 };
const unsigned char pro_group_addr[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x08 };
const unsigned char igmp_group_addr[6] = { 0x01, 0x00, 0x5e, 0x00, 0x00, 0x00 };
const unsigned char zero_mac_addr[6] = { 0, 0, 0, 0, 0, 0 };
const unsigned char cfm_ccm_addr[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x30};
const unsigned char cfm_ltm_addr[6] = { 0x01, 0x80, 0xc2, 0x00, 0x00, 0x38};
const unsigned char reserved_addr_start[6] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x00};
const unsigned char reserved_addr_end[6] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x0f};
const unsigned char rpvst_group_addr[6] = { 0x01, 0x00, 0x0c, 0xcc, 0xcc, 0xcd };
const unsigned char pbb_oui[3] = { 0x01, 0x1e, 0x83 };
const unsigned char elmi_group_addr[ETH_MAC_LEN] = {0x01, 0x80, 0xc2, 0x00, 0x00, 0x07};

static long *delivered_port_map = 0;

/* This function checks if the given mac address is a local address */

static bool_t
br_vlan_is_dest_mac_addr_local ( struct sk_buff *skb )
{
  unsigned char *dest_addr = skb->mac_header;
  vid_t vid = VLAN_DEFAULT_VID;
  struct apn_bridge_fdb_entry *fdb_entry;
  struct apn_bridge_port *port;
  struct apn_bridge *br;
  bool_t is_local = BR_FALSE;

  port = (struct apn_bridge_port *) skb->dev->apn_fwd_port;
  br = port->br;

  /* FUTURE: temp hack MUST be fixed once eth port list data structure in bridge
     is redesigned */
  vid = VLAN_DEFAULT_VID;
  fdb_entry = br_fdb_get ( br, dest_addr, vid, vid );

  if (fdb_entry)
    {
      is_local = fdb_entry->is_local;
      br_fdb_put (fdb_entry);
    }
  return is_local;
}

static bool_t
br_vlan_is_dest_mac_addr_vlan_local (struct sk_buff *skb,
                                     struct vlan_info_entry *vinfo)
{
  unsigned char *dest_addr = skb->mac_header;

  if (!memcmp (dest_addr, vinfo->dev->dev_addr, vinfo->dev->addr_len))
    return BR_TRUE;
  else
    return BR_FALSE;
}

/* This function passes a frame up the VI */
static void
br_vlan_pass_frame_up (struct vlan_info_entry *vinfo, struct sk_buff *skb)
{
  br_frame_t frame_type;
  struct net_device *indev;
  struct apn_bridge_port *port;
  unsigned char *dst_addr;
  struct net_device_stats *stats = &vinfo->net_stats;

  if (skb == NULL)
    return;

  dst_addr = skb->mac_header;

  /* Check if the frame is a tagged or untagged or priority tagged frame */
  BDEBUG ("skb src " MAC_ADDR_FMT " dst " MAC_ADDR_FMT " VI: %s\n",
          MAC_ADDR_PRINT (skb->mac.ethernet->h_source),
          MAC_ADDR_PRINT (dst_addr), vinfo->dev->name);

  /* Update statistics */
  stats->rx_packets++;
  stats->rx_bytes += skb->len;
  port = skb->dev->apn_fwd_port;

  /* Check if the frame is a tagged or untagged or priority tagged frame */
  if (port->port_type == PRO_NET_PORT
      || port->port_type == CUST_NET_PORT)
    frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_STAG);
  else
    frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_CTAG);

  indev = skb->dev;

  /* Important! Change skb-dev to this VI */
  skb->dev = vinfo->dev;

  /* Check frame type to see whether we need to pop VLAN headers */
  switch (frame_type)
    {
    case UNTAGGED:
      break;
    case PRIORITY_TAGGED:
    case TAGGED:
      BDEBUG("Pop vlan tag header for priority tagged/tagged frame\n");
      skb = br_vlan_pop_tag_header (skb);

      if (!skb)
        {
          stats->rx_dropped++;
          stats->rx_errors++;
          return;
        }
      /* Since br_vlan_pop_tag_header() has popped the VLAN tag header
       * and pushed a regular Ethernet header, we need to pull it so
       * that the eth_type_trans() logic below works
       */
      skb_pull (skb, skb->dev->hard_header_len);

      break;
    default:
      /* Error */
      break;
    }

  /* Do protocol type translation */
  skb->pkt_type = PACKET_HOST;
  skb_push (skb, skb->dev->hard_header_len);
  skb->protocol = eth_type_trans (skb, skb->dev);

  /* Handle packet type */
  switch (skb->pkt_type)
    {
    case PACKET_MULTICAST:
      stats->multicast++;
      break;
    default:
      break;
    }

  netif_rx (skb);

  skb->dev->last_rx = jiffies;

  return;
}

/* This function schedules a frame destined for the local system with the OS
   for further processing.  */
static void
br_pass_frame_up ( struct apn_bridge *br, struct sk_buff *skb )
{
  extern int stp_rcv ( struct sk_buff *skb,
                       struct apn_bridge *bridge, struct net_device *port );

  /* Here we need to peek at the frame and determine if it
     goes to the stp socket or is pushed back into the IP stack. */
  BDEBUG ("In pass frame up \n");

  stp_rcv ( skb, br, skb->dev );
}

/* Called for passing the frame up to garp */
static void
br_pass_frame_up_garp ( struct apn_bridge *br, struct sk_buff *skb )
{
  extern int garp_rcv ( struct sk_buff *skb,
                        struct apn_bridge *bridge, struct net_device *port );

  BDEBUG ("In pass frame up to GARP\n");

  garp_rcv ( skb, br, skb->dev );
}


/* Called for passing the frame up to igmp_snooping */
static void
br_pass_frame_up_igmp_snoop ( struct apn_bridge *br, struct sk_buff *skb )
{
  extern int igmp_snoop_rcv ( struct sk_buff *skb,
                              struct apn_bridge *bridge, struct net_device *port );

  BDEBUG ("In pass frame up to IGMP SNOOP\n");

  igmp_snoop_rcv ( skb, br, skb->dev );
}

#ifdef HAVE_CFM
/* Called for passing the frame up to cfm */
static void
br_pass_frame_up_cfm ( struct apn_bridge *br, struct sk_buff *skb )
{
  extern int cfm_rcv ( struct sk_buff *skb, struct apn_bridge *bridge,
                       struct net_device *port );

  BDEBUG ("In pass frame up to CFM\n");

  cfm_rcv (skb, br, skb->dev );
}
#endif /* HAVE_CFM */

#ifdef HAVE_ELMID
/* Called for passing the frame up to ELMI */
static void
br_pass_frame_up_elmi (struct apn_bridge *br, struct sk_buff *skb )
{
  extern int elmi_rcv (struct sk_buff *skb, struct apn_bridge *bridge,
                       struct net_device *port);

  BDEBUG ("In pass frame up to ELMI\n");

  elmi_rcv (skb, br, skb->dev );
}
#endif /* HAVE_ELMID */

/* Handle dynamic forwarding for multicast frames. */
static br_result_t
br_dynamic_forward_multicast_frame ( struct apn_bridge *br,
                                     struct apn_bridge_fdb_entry *fdb_entry,
                                     struct sk_buff *skb, vid_t cvid, vid_t svid )
{
  vid_t vid;
  struct sk_buff *skb2;
  char *config_ports = 0;
  char *pvlan_config_ports = 0;
  bool_t is_egress_tagged = BR_FALSE;
  vid_t port_default_vid[BR_MAX_PORTS];
  struct vlan_info_entry * vlan = NULL;
  struct apn_multicast_port_list *mport;

  if ( br->is_vlan_aware )
    {
      vlan = br->vlan_info_table[cvid];

      if (!vlan)
        return BR_ERROR;

      if ((vlan) && (vlan->pvlan_configured))
        {
          pvlan_config_ports = br_pvlan_get_configure_ports (br, cvid,
                                                             port_default_vid);

          if (!pvlan_config_ports)
            return BR_ERROR;

          mport = fdb_entry->mport;

          while ( mport )
            {
              /* Check if the ingress vid (untagged (classified)/ tagged) is
                 different from the egress vid */
              if ( ( fdb_entry->vid != vid ) && mport->port->enable_vid_swap )
                {
                  vid = fdb_entry->vid;
                }

              is_egress_tagged =
                IS_VLAN_EGRESS_TAGGED ( pvlan_config_ports[mport->port->port_id] );

              if ( ( skb2 = skb_clone ( skb, GFP_ATOMIC ) ) == NULL )
                return BR_NORESOURCE;
              if (IS_VLAN_PORT_CONFIGURED
                  (pvlan_config_ports[mport->port->port_id]))
                {
                  br_forward ( mport->port,
                               port_default_vid [mport->port->port_id],
                               skb2, is_egress_tagged );
                }
              mport = mport->next;
            }
          kfree (pvlan_config_ports);
        }
      else
        {
          mport = fdb_entry->mport;

          while ( mport )
            {
              if (mport->port->port_type == CUST_NET_PORT
                  || mport->port->port_type == PRO_NET_PORT)
                {
                  vid = svid;
                  config_ports = br_vlan_get_vlan_config_ports
                                                  ( br, SERVICE_VLAN, svid );
                }
              else
                {
                  vid = cvid;
                  config_ports = br_vlan_get_vlan_config_ports
                                                 ( br, CUSTOMER_VLAN, cvid );
                }

               /* Check if the egress side the frame should be tagged */
              is_egress_tagged =
                IS_VLAN_EGRESS_TAGGED ( config_ports[mport->port->port_id] );

              if ( ( skb2 = skb_clone ( skb, GFP_ATOMIC ) ) == NULL )
                return BR_NORESOURCE;

              br_forward ( mport->port, vid, skb2, is_egress_tagged );

              mport = mport->next;
            }
        }
    } /* end of vlan_aware */
  else
    {
      mport = fdb_entry->mport;
      while ( mport )
        {
          if ( ( skb2 = skb_clone ( skb, GFP_ATOMIC ) ) == NULL )
            return BR_NORESOURCE;

          br_forward ( mport->port, cvid, skb2, is_egress_tagged );

          mport = mport->next;
        }
    }

  kfree_skb (skb);
  return BR_MATCH;
}

/* Handle dynamic forwarding of unicast frames. */
static br_result_t
br_dynamic_forward_unicast_frame ( struct apn_bridge * br,
                                   struct apn_bridge_fdb_entry * fdb_entry,
                                   struct sk_buff * skb, vid_t cvid, vid_t svid )
{
  vid_t vid;
  char *config_ports = 0;
  char *pvlan_config_ports = 0;
  bool_t is_egress_tagged = BR_FALSE;
  struct vlan_info_entry *vlan = NULL;
  vid_t port_default_vid[BR_MAX_PORTS];

  BDEBUG (" br_dynamic_forward_unicast_frame cvid %u svid %u \n", cvid, svid);

  if ( br->is_vlan_aware )        /* vlan-aware bridge */
    {
      if (fdb_entry->port->port_type == CUST_NET_PORT
              || fdb_entry->port->port_type == PRO_NET_PORT)
        {
          vid = svid;
          vlan = br->svlan_info_table[vid];
        }
      else
        {
          vid = cvid;
          vlan = br->vlan_info_table[vid];
        }

      if (!vlan)
        {
          BDEBUG (" vlan_info_table NULL \n");
          return BR_ERROR;
        }

      if ((vlan) && (vlan->pvlan_configured))
        {
          pvlan_config_ports = br_pvlan_get_configure_ports (br, vid,
                                                             port_default_vid);

          if (!pvlan_config_ports)
            return BR_ERROR;

          is_egress_tagged =
            IS_VLAN_EGRESS_TAGGED ( pvlan_config_ports[fdb_entry->port->port_id] );

          if (IS_VLAN_PORT_CONFIGURED
              (pvlan_config_ports[fdb_entry->port->port_id]))
            {
              br_forward ( fdb_entry->port,
                           port_default_vid [fdb_entry->port->port_id],
                           skb, is_egress_tagged );
            }

          kfree (pvlan_config_ports);
        }
      else
        {

          if (fdb_entry->port->port_type == CUST_NET_PORT
              || fdb_entry->port->port_type == PRO_NET_PORT)
            config_ports = br_vlan_get_vlan_config_ports ( br, SERVICE_VLAN,
                                                           vid );
          else
            config_ports = br_vlan_get_vlan_config_ports ( br, CUSTOMER_VLAN,
                                                           vid );

          if ( !config_ports )
            {
              BDEBUG (" config_ports NULL \n");
              return BR_ERROR;
            }

          /* Check if the egress side the frame should be tagged */
          is_egress_tagged =
            IS_VLAN_EGRESS_TAGGED ( config_ports[fdb_entry->port->port_id] );
          br_forward ( fdb_entry->port,
                       vid,
                       skb, is_egress_tagged );
        }

    }                                /* end of if vlan-aware bridge */
  else
    {
      br_forward ( fdb_entry->port, vid, skb, is_egress_tagged );
    }

  return BR_MATCH;
}

/* Handle dynamic forwarding. Called under Bridge Lock. */
br_result_t
br_dynamic_forward_frame ( struct sk_buff *skb, struct apn_bridge *br,
                           vid_t cvid, vid_t svid )
{
  int ret;
  struct vlan_info_entry *vlan = NULL;
  struct apn_bridge_fdb_entry *fdb_entry = NULL;
  unsigned char *dest_addr = skb->mac_header;

  if (br->is_vlan_aware)
    {

      vlan = br->vlan_info_table[cvid];

      if ((vlan) && (vlan->pvlan_configured))
        {
          BDEBUG (" pvlan configured \n");

          fdb_entry = br_fdb_get_pvlan_entry (br, dest_addr, cvid );

          if (fdb_entry == NULL)
            {
              BDEBUG (" fdb_entry is null \n");
              return BR_NOMATCH;
            }

        }
      else
        {
          fdb_entry = br_fdb_get ( br, dest_addr, cvid, svid );
        }
    }
  else
    {
      fdb_entry = br_fdb_get ( br, dest_addr, cvid, svid );
    }

  if ( fdb_entry == NULL )
    {
      BDEBUG (" fdb_entry is null \n");
      return BR_NOMATCH;
    }

  BDEBUG ( "In br_dynamic_forward_frame cvid %u svid %u\n", cvid, svid );

  if ( !memcmp ( fdb_entry->addr.addr, igmp_group_addr, 3 ) )
    ret = br_dynamic_forward_multicast_frame ( br, fdb_entry, skb, cvid, svid );
  else
    ret = br_dynamic_forward_unicast_frame ( br, fdb_entry, skb, cvid, svid );

  br_fdb_put ( fdb_entry );

  return ret;
}

/* Flood a frame onto every port in the port map */

static __inline__ br_result_t
br_flood_restricted ( struct apn_bridge *br, struct sk_buff *skb, vid_t vid,
                      long *filter_portmap )
{
  bool_t is_egress_tagged = BR_FALSE;
  struct sk_buff *skb2;
  vid_t port_default_vid[BR_MAX_PORTS];
  struct vlan_info_entry *vlan = NULL;

  if ( br->is_vlan_aware )        /* vlan-aware bridge */
    {
      char *config_ports = 0;
      struct apn_bridge_port *port = br->port_list;
      int port_no;
      vlan = br->vlan_info_table[vid];
      char *pvlan_config_ports;

      if (!vlan)
        return BR_ERROR;

      if ((vlan) && (vlan->pvlan_configured))
        {
          pvlan_config_ports = br_pvlan_get_configure_ports (br, vid,
                                                             port_default_vid);
          if (!pvlan_config_ports)
            return BR_ERROR;
          while (port != NULL)
            {
              if ( IS_VLAN_PORT_CONFIGURED ( pvlan_config_ports[port->port_id] ) )
                {
                  BDEBUG (" port_id = %d filter_port_map = %d \n",
                          port->port_id, BR_GET_PMBIT ( port->port_id, filter_portmap ));
                  if ( !BR_GET_PMBIT ( port->port_id, filter_portmap ) )
                    {
                      /* Clone the buffer and forward to the indicated port */

                      if ( ( skb2 = skb_clone ( skb, GFP_ATOMIC ) ) == NULL )
                        {
                          br->statistics.tx_dropped++;
                          kfree_skb ( skb );
                          return BR_NORESOURCE;
                        }

                      is_egress_tagged =
                        IS_VLAN_EGRESS_TAGGED ( pvlan_config_ports[port->port_id] );
                      br_forward ( port,
                                   port_default_vid [port->port_id],
                                   skb2, is_egress_tagged );
                    }
                }
              port = port->next;
            } /* end of while */
        }
      else
        {
          config_ports = br_vlan_get_vlan_config_ports ( br, CUSTOMER_VLAN,
                                                         vid );

          if ( !config_ports )
              {
              return BR_ERROR;
            }

          while (port != NULL)
            {
              if ( IS_VLAN_PORT_CONFIGURED ( config_ports[port->port_id] ) )
                {
                  BDEBUG (" port_id = %d filter_port_map = %d \n",
                          port->port_id, BR_GET_PMBIT ( port->port_id, filter_portmap ));
                  if ( !BR_GET_PMBIT ( port->port_id, filter_portmap ) )
                    {
                      /* Clone the buffer and forward to the indicated port */

                        if ( ( skb2 = skb_clone ( skb, GFP_ATOMIC ) ) == NULL )
                        {
                          br->statistics.tx_dropped++;
                          kfree_skb ( skb );
                          return BR_NORESOURCE;
                        }

                        is_egress_tagged =
                          IS_VLAN_EGRESS_TAGGED ( config_ports[port->port_id] );
                        br_forward ( port, vid, skb2, is_egress_tagged );
                    }
                }
              port = port->next;
            }
        }
    }                                /* end of vlan-aware bridge */
  else                                /* transparent bridge */
    {
      struct apn_bridge_port *port = br->port_list;

      while ( port )
        {
          if ( !BR_GET_PMBIT ( port->port_id, filter_portmap ) )
            {
              /* Clone the buffer and forward to the indicated port */
              struct sk_buff *skb2;

              if ( ( skb2 = skb_clone ( skb, GFP_ATOMIC ) ) == NULL )
                {
                  br->statistics.tx_dropped++;
                  kfree_skb ( skb );
                  return BR_NORESOURCE;
                }

              /* the vid will be VLAN_NULL_VID  and the is_egress_tagged will
                 be BR_FALSE for transparent bridge */
              br_forward ( port, vid, skb2, is_egress_tagged );
            }
        }
    }                                /* end of else transparent bridge */

  kfree ( skb );
  return BR_NOERROR;
}

/* Handle static forwarding. Called under Bridge Lock. */
br_result_t
br_static_forward_frame ( struct sk_buff *skb, struct apn_bridge *br,
                          vid_t cvid, vid_t svid )
{
  vid_t vid;
  vid_t port_default_vid[BR_MAX_PORTS];
  struct static_entries static_entries;
  unsigned char *dest_addr = skb->mac_header;
  struct vlan_info_entry *pvlan = br->vlan_info_table[cvid];
  struct apn_bridge_port *port = (struct apn_bridge_port *)(skb->dev->apn_fwd_port);

  if ((br->is_vlan_aware) && (pvlan) && (pvlan->pvlan_configured))
    {
      BDEBUG (" PVLAN configured on vid %d \n", cvid);

      if (br_fdb_get_static_pvlan_fdb_entries (br, dest_addr,
                                               cvid, &static_entries ) == BR_NOMATCH )
        {
          BDEBUG (" br_fdb_get_static_pvlan_fdb_entries = BR_NOMATCH \n");
          return BR_NOMATCH;
        }
    }
  else if ( br_fdb_get_static_fdb_entries ( br, dest_addr,
                                            cvid, svid,
                                            &static_entries ) == BR_NOMATCH )
    {
      BDEBUG ("br_fdb_get_static_fdb_entries nomatch \n");
      return BR_NOMATCH;
    }

  if ( static_entries.is_forward )        /* handle forwarding entries */
    {
      struct sk_buff *skb2;
      struct static_entry *current_entry = static_entries.forward_list;
      bool_t is_egress_tagged = BR_FALSE;

      while ( current_entry )
        {
          /* Never forward a frame onto it's own source port */
          if ( current_entry->port != port )
            {
              if ( ( skb2 = skb_clone ( skb, GFP_ATOMIC ) ) == NULL )
                {
                  br->statistics.tx_dropped++;
                  return BR_NORESOURCE;
                }

              BDEBUG ( "static forward on port %d\n",
                       current_entry->port->port_no );

              if ( br->is_vlan_aware )        /* vlan-aware bridge */
                {
                  char *config_ports = 0;
                  char *pvlan_config_ports = 0;

                  if ((pvlan) && (pvlan->pvlan_configured))
                    {
                      pvlan_config_ports = br_pvlan_get_configure_ports
                                              (br, cvid, port_default_vid);

                      if (!pvlan_config_ports)
                        return BR_ERROR;

                      is_egress_tagged =
                        IS_VLAN_EGRESS_TAGGED ( pvlan_config_ports
                                                [current_entry->port->port_id] );

                      if (IS_VLAN_PORT_CONFIGURED
                          (pvlan_config_ports[current_entry->port->port_id]))
                        {
                          br_forward ( current_entry->port,
                                       port_default_vid [current_entry->port->port_id],
                                       skb2, is_egress_tagged );
                        }

                    }
                  else
                    {


                      if (current_entry->port == NULL)
                        continue;

                      /* Check if egress_tagged */

                      if (current_entry->port->port_type == CUST_NET_PORT
                          || current_entry->port->port_type == PRO_NET_PORT)
                        {
                          config_ports = br_vlan_get_vlan_config_ports ( br,
                                                          SERVICE_VLAN,
                                                          svid );
                          vid = svid;
                        }
                      else
                        {
                          config_ports = br_vlan_get_vlan_config_ports ( br,
                                                          CUSTOMER_VLAN,
                                                          cvid );
                          vid = cvid;
                        }

                      if ( !config_ports )
                        {
                          return BR_ERROR;
                        }

                      is_egress_tagged =
                        IS_VLAN_EGRESS_TAGGED ( config_ports
                                                [current_entry->port->port_id] );

                      br_forward ( current_entry->port, vid, skb2, is_egress_tagged );
                    }
                }
              else
                {
                  br_forward ( current_entry->port, cvid, skb2, is_egress_tagged );
                }
            }
          if ( delivered_port_map && current_entry->port )
            BR_SET_PMBIT (current_entry->port->port_id, delivered_port_map);
          current_entry = current_entry->next;
        }                        /* end of while current entry */
    }
  else if ( static_entries.is_filter )        /* handle filtering entries */
    {
      /* There are no forwarding entries - only filtering entries. */
      struct sk_buff *skb2;

      BDEBUG ( "flood restricted\n" );

      if ( ( skb2 = skb_clone ( skb, GFP_ATOMIC ) ) == NULL )
        {
          br->statistics.tx_dropped++;
          return BR_NORESOURCE;
        }

      /* Check if there are any dynamic entries for this frame.  If so,
         forward based on the dynamic entry */
      if ( br_dynamic_forward_frame ( skb2, br, cvid, svid ) == BR_NOMATCH )
        {
          /* Flood the frame to all ports except the ones with filter entries */
          if ( br_flood_restricted ( br, skb2, cvid,
                                     static_entries.filter_portmap ) !=
               BR_NOERROR )
            {
              return BR_NORESOURCE;
            }
        }
    }
  else
    {
      BWARN
        ( "Static entry for addr %.02x%.02x.%.02x%.02x.%.02x%.02x is neither forward nor filter\n",
          dest_addr[0], dest_addr[1], dest_addr[2], dest_addr[3], dest_addr[4],
          dest_addr[5] );
      return BR_ERROR;
    }

  return BR_MATCH;
}

static int
br_gmrp_serv_req_forward_frame (struct sk_buff *skb, struct apn_bridge *br,
                                vid_t cvid, vid_t svid, br_result_t found_match)
{
  vid_t vid;
  int is_present ;
  char* config_ports = 0 ;
  bool_t is_egress_tagged;
  struct apn_bridge_port *port = br->port_list;

  is_present = (found_match == BR_MATCH) ? 1 : 0;

  while ( port )
    {
      struct sk_buff *skb2;

      if (BR_GET_PMBIT (port->port_id, delivered_port_map))
        {
          port = port->next;
          continue;
        }

      BDEBUG (" port %d fwdall serv req is %d \n ",
              port->port_id, port->gmrp_serv_req_fwd_all );
      BDEBUG (" port %d fwdunreg serv req is %d \n ",
              port->port_id, port->gmrp_serv_req_fwd_unreg );

      if ( ( skb2 = skb_clone ( skb, GFP_ATOMIC ) ) == NULL )
        {
          br->statistics.tx_dropped++;
          return BR_NORESOURCE;
        }

      /* Check if egress_tagged */

      if (port->port_type == CUST_NET_PORT
          || port->port_type == PRO_NET_PORT)
        {
           vid = svid;
           config_ports = br_vlan_get_vlan_config_ports ( br, SERVICE_VLAN,
                                                          svid );
        }
      else
        {
           vid = svid;
           config_ports = br_vlan_get_vlan_config_ports ( br, CUSTOMER_VLAN,
                                                          cvid );
        }

      if ( !config_ports )
        {
          return 0;
        }

      is_egress_tagged = IS_VLAN_EGRESS_TAGGED ( config_ports
                                                 [port->port_id] );


      if (BR_SERV_REQ_VLAN_IS_SET(port->gmrp_serv_req_fwd_all, vid) )
        {
          br_forward (port, vid, skb2, is_egress_tagged);
          BR_SET_PMBIT (port->port_id, delivered_port_map);
        }
      else  if ((BR_SERV_REQ_VLAN_IS_SET (port->gmrp_serv_req_fwd_unreg, vid))  && (!is_present))
        {
          br_forward (port, vid, skb2, is_egress_tagged);
          BR_SET_PMBIT (port->port_id, delivered_port_map);
        }
      port = port->next;

    }

  return 0;
}


/* This function performs the actual frame forwarding.
   It finds the output ports by examining the static and dynamic FDBs.
   Frames addressed to the local station are passed up to the OS.
   Broadcast frames are broadcast.                            */

static int
br_handle_frame_finish ( struct sk_buff *skb, vid_t cvid, vid_t svid )
{
  int passedup = 0;
  struct apn_bridge *br;
  struct vlan_info_entry *entry;
  int gmrp_multicast_serv_req_result;
  br_result_t static_result, static_multicast_result;
  struct apn_bridge_port *port = skb->dev->apn_fwd_port;

  if ( port == NULL )
    {
      kfree_skb ( skb );
      return 0;
    }

  br = port->br;

  if (br == NULL)
    {
      kfree_skb (skb);
      return 0;
    }
  BR_READ_LOCK ( &br->lock );

  BDEBUG ( "in handle frame finish %u cvid %u svid\n", cvid, svid );
  /* skb_dump (skb);  */

  /* gmrp Extended filtering services */
  if (( skb_dst_mac_addr(skb) & 1)
      && (br->garp_config & APNBR_GARP_GMRP_CONFIGURED)
      && (br->ext_filter) ) /* dest addr is multicast, gmrp enabled and ext filter enabled */
    {
      if ((delivered_port_map = kmalloc ( (sizeof (long) * (PORTMAP_SIZE +1)),
                                          GFP_ATOMIC)) == NULL)
        {
          kfree_skb (skb);
          BR_READ_UNLOCK ( &br->lock );
          return 0;
        }

      static_multicast_result = br_static_forward_frame (skb, br, cvid, svid);

      if ( ( static_multicast_result == BR_ERROR ) || ( static_multicast_result == BR_NORESOURCE ) )
        {
          BDEBUG ( "In Multicast forward: static_multicast_result = %d\n", static_multicast_result );
          kfree_skb (skb);
          BR_READ_UNLOCK ( &br->lock );
          return 0;
        }

      gmrp_multicast_serv_req_result = br_gmrp_serv_req_forward_frame (skb, br,
                                                    cvid, svid,
                                                    static_multicast_result);

      if (delivered_port_map)
        BR_CLEAR_PORT_MAP (delivered_port_map);

      kfree (delivered_port_map);
      kfree_skb ( skb );
      BR_READ_UNLOCK ( &br->lock);
      return 0;
    }


  /* To allow for filtering broadcast frames statically, check in static
     fdb before checking for broadcast */
  static_result = br_static_forward_frame ( skb, br, cvid, svid );

  if ( ( static_result == BR_ERROR ) || ( static_result == BR_NORESOURCE ) )
    {
      kfree_skb (skb);
      BR_READ_UNLOCK ( &br->lock );
      return 0;
    }

  if ( br_dynamic_forward_frame ( skb, br, cvid, svid ) == BR_NOMATCH )
    {
      if ( static_result == BR_NOMATCH )
        {
          BDEBUG ( "no entry found - broadcast (flood forward)\n" );
          br_flood_forward ( br, skb, cvid, svid, passedup );
        }
      else
        {
          /* Must have ben forwarded by static fdb */
          kfree_skb ( skb );
        }
    }

  BR_READ_UNLOCK ( &br->lock );
  return 0;
}

/* This function handles frames passed in by the OS.
   Learning and input port state filtering are performed here. */

int
br_handle_frame ( struct sk_buff *skb )
{
  int learn, forward;
  struct sk_buff *nskb;
  unsigned short proto;
  struct apn_bridge *br;
  struct vlan_info_entry *vinfo = NULL;
  struct vlan_info_entry **vlan_info_table;
  unsigned short instance = BR_INSTANCE_COMMON;
  unsigned char *dest_addr = skb->mac_header;
  unsigned char *src_addr = &skb_src_mac_addr(skb);
  struct apn_bridge_port *port =
                          (struct apn_bridge_port *)(skb->dev->apn_fwd_port);

  int is_local = 0;

  /* vid will be used ONLY for vlan-aware bridges and for transparent bridges
     the value will always be VLAN_NULL_VID */
  vid_t cvid = VLAN_NULL_VID;
  vid_t svid = VLAN_NULL_FID;
  vid_t inner_tag = VLAN_NULL_VID;
  vid_t outer_tag = VLAN_NULL_FID;


  /* No input port - discard frame */
  if ( port == NULL )
    {
      kfree_skb ( skb );
      return -1;
    }

  BDEBUG ( "Frame recvd on ifindex %d\n", port->port_no );
  br = port->br;
  if (br == NULL)
    {
      kfree_skb (skb);
      return -1;
    }
  BR_READ_LOCK ( &br->lock );

  if ( ( skb->dev->apn_fwd_port == NULL ) ||        /* Port may go bye-bye during lock */
       ( !( br->flags & APNBR_UP ) ) ||        /* Interface not up, discard frame */
       ( skb_src_mac_addr(skb) & 1 ) )        /* Src address is multicast? */
    {
      BDEBUG ( "interface not up or bridge is not up or src multicast\n" );
      BR_READ_UNLOCK ( &br->lock );
      kfree_skb ( skb );
      return -1;
    }

  if (port->port_type == PRO_NET_PORT
      || port->port_type == CUST_NET_PORT)
    {
      proto = ETH_P_8021Q_STAG;
      vlan_info_table = br->svlan_info_table;
    }
  else
    {
      proto = ETH_P_8021Q_CTAG;
      vlan_info_table = br->vlan_info_table;
    }

  if (skb_shared (skb) || skb_cloned (skb))
    {
      nskb = skb_copy (skb, GFP_ATOMIC);
      if (nskb == NULL)
        {
          kfree_skb (skb);
          BR_READ_UNLOCK ( &br->lock );
          return -1;
        }

      kfree_skb (skb);
      skb = nskb;
    }

  if ( br->is_vlan_aware )        /* vlan-aware bridge */
    {

#ifdef HAVE_ELMID
        /* Lift ELMI frames on Customer-edge port(UNI-N) or
         * on Access/Trunk/Hybrid ports (UNI-C)
         */
        /* Check for untagged frames done in elmi_rcv */
      if ((port->port_type == CUST_EDGE_PORT || 
           port->port_type == ACCESS_PORT ||
           port->port_type == TRUNK_PORT ||
           port->port_type == HYBRID_PORT) && 
          (br_vlan_get_nondot1q_ether_type (skb) == ETH_P_ELMI) &&
          (!memcmp (dest_addr, elmi_group_addr, ETH_MAC_LEN))) 
        {
          br_pass_frame_up_elmi (br, skb);
          read_unlock (&br->lock);
          kfree_skb (skb);
          return 0;
        }
#endif /* HAVE_ELMID */

       br_vlan_translate_ingress_vid ( port, skb );

      /* Check if the frame should be processed or discarded after applying the
         ingress rules */
      if ( !br_vlan_apply_ingress_rules ( proto, skb, &outer_tag) )
        {
          BDEBUG ( "Apply ingress rules failed for this frame %d \n", outer_tag);
          BR_READ_UNLOCK ( &br->lock );
          kfree_skb ( skb );
          return -1;
        }

      /* Check for VLAN. */
      if (outer_tag > 0)
        {
          /* If VLAN is not created and we get a frame just discard it. */
          if ((vinfo = vlan_info_table[outer_tag]) == NULL)
            {
              BR_READ_UNLOCK (&br->lock);
              kfree_skb (skb);
              return -1;
            }
        }

      if (port->port_type == CUST_EDGE_PORT)
        {
          cvid = outer_tag;
          svid = br_vlan_svid_get (port, cvid);

          if (svid == VLAN_NULL_VID)
            {
              BDEBUG ( " No registration entry for the cvid %d for port %d\n",
                       cvid, port->port_no );
              read_unlock ( &br->lock );
              kfree_skb ( skb );
              return -1;
            }
        }
      else if ((port->port_type == PRO_NET_PORT
               || port->port_type == CUST_NET_PORT)
               && (br->num_ce_ports || br->num_cnp_ports))
        {
          svid = outer_tag;
          br_vlan_apply_ingress_rules ( ETH_P_8021Q_CTAG, skb, &cvid);
        }
      else
        cvid = svid = outer_tag;

      instance = vlan_info_table[outer_tag]->instance;

      BDEBUG("applied ingress rules and the frames vid(%u) svid(%u) instance (%d)\n", cvid, svid,instance);
    } /* end of if vlan-aware bridge */

  /* This part of the code will be common to both vlan-aware bridge and
     transparent bridge */
  /* Learning Process:
     transparent bridge: learn <src MAC addr, ingress port>
     vlan-aware bridge : learn <src MAC addr, ingress port, ingress VID/FID> */
  /* for now we just check the status of instance 0 */

  if (port->port_type == CUST_EDGE_PORT)
    {
      learn = ( br_cvlan_state_flags_get (port, cvid) & APNFLAG_LEARN ) ? 1 : 0;
      forward = ( br_cvlan_state_flags_get (port, cvid) & APNFLAG_FORWARD )
                                                                     ? 1 : 0;
    }
  else
    {
      learn = ( port->state_flags[instance] & APNFLAG_LEARN ) ? 1 : 0;
      forward = ( port->state_flags[instance] & APNFLAG_FORWARD ) ? 1 : 0;
    }

  BDEBUG ( "learn %d and forward %d for instance %d \n",
           learn, forward, instance );

  if ( ( learn ) && ( br->flags & APNBR_LEARNING_ENABLED ) )
    {
      /* Learning must be enabled by mgmt - true by default */
      br_fdb_insert ( br, port, src_addr, cvid, svid, is_local, BR_TRUE );
    }

  /* If the VLAN device exists and is up, handle the frame */
  if (vinfo && vinfo->dev && forward)
    {
      struct sk_buff *skb2;   /* Copy */

      /* Check whether incoming port is in VLAN. A similar check is done
       * in br_vlan_apply_ingress_rules(), but only when
       * port->enable_ingress_filter is on.
       * Here we do the check again to make sure that we pass up frames to
       * VI only when the incoming port is a VLAN member.
       */
      if (!br_vlan_is_port_in_vlans_member_set (vinfo->br,
                                                vinfo->vid, port->port_no))
        {
          BDEBUG ("VI %s does not contain the port %s its its member set. Not passing frame up\n",
                  vinfo->dev->name, port->dev->name);
          goto skip_vi;
        }

      /* Check whether the VLAN is up */
      if (!(vinfo->dev->flags & IFF_UP))
        {
          goto skip_vi;
        }

      /* Check if we have a MAC address assigned to the VI */
      if (!memcmp (vinfo->dev->dev_addr, zero_mac_addr, vinfo->dev->addr_len))
        {
          BDEBUG ("VI %s does not have a MAC address. Not passing frame up\n", vinfo->dev->name);
          goto skip_vi;
        }

      /* We pass the frame up to VI if the interface is in PROMICUOS or
       * ALL_MULTI mode, or if the destination mac address is multicast or
       * broadcast or if the destination mac address is local.
       * In all cases, we have to make a copy of the skb so that further
       * processing in the bridge is not affected.
       */

      /* PROMISCUOUS or ALL_MULTI */
      if (vinfo->dev->flags & (IFF_PROMISC | IFF_ALLMULTI))
        {
          skb2 = skb_copy (skb, GFP_ATOMIC);
          if (skb2 != NULL)
            {
              br_vlan_pass_frame_up (vinfo, skb2);
              goto skip_vi;
            }
        }

      /* Broadcast or IP Multicast */
      if ((!memcmp (dest_addr, igmp_group_addr, 3)) ||
          (!memcmp (dest_addr, vinfo->dev->broadcast, vinfo->dev->addr_len)))
        {
          skb2 = skb_copy (skb, GFP_ATOMIC);
          if (skb2 != NULL)
            {
              br_vlan_pass_frame_up (vinfo, skb2);
              goto skip_vi;
            }
        }

      /* Locally destined packets */
      if (br_vlan_is_dest_mac_addr_vlan_local (skb, vinfo))
        {
          /* Even though the frame is locally destined, we have to make a copy
           * since it can be passed up to isl_rcv() in br_handle_frame_finish()
           */
          skb2 = skb_copy (skb, GFP_ATOMIC);
          if (skb2 != NULL)
            {
              br_vlan_pass_frame_up (vinfo, skb2);
              goto skip_vi;
            }
        }
    }

 skip_vi:
  if ( (!memcmp ( dest_addr, gmrp_group_addr, 6 )) ||
       (!memcmp ( dest_addr, gvrp_group_addr, 6 )) )
    {
      /* For gvrp only untagged frames will be processed. Sec 11.2.3.3 */
      if ((!memcmp ( dest_addr, gvrp_group_addr, 6 )) &&
          (br_vlan_get_frame_type (skb, proto) != UNTAGGED) &&
          (br->type != BR_TYPE_PROVIDER_RSTP) &&
          (br->type != BR_TYPE_PROVIDER_MSTP))
        {
          kfree (skb);
          BR_READ_UNLOCK ( &br->lock );
          return -1;
        }

      /* Currently the other multicast frames are also being passed up
         to br_handle_frame_finish for flooding. Later when we have a
         configurable group address, we should flood the other entries  */
      BDEBUG ( "BPDU received  %d\n", port->state[instance] );
      if (((!memcmp ( dest_addr, gmrp_group_addr, 6 )) &&
           (br->garp_config & APNBR_GARP_GMRP_CONFIGURED
            || br->garp_config & APNBR_MRP_MMRP_CONFIGURED
            || port->port_type == CUST_EDGE_PORT))||
          ((!memcmp ( dest_addr, gvrp_group_addr, 6 )) &&
           (br->garp_config & APNBR_GARP_GVRP_CONFIGURED
            || br->garp_config & APNBR_MRP_MVRP_CONFIGURED)))
        {
          br_pass_frame_up_garp ( br, skb );
          kfree_skb(skb);
        }
      else
        {
          /* The bridge is either not configured for gvrp and packet received
             is gvrp or the bridge is not configured for gmrp
             and packet received is gmrp or the vice versa or the bridge is
             neither gvrp or gmrp configured, we forward the packet
             IEEE 802.1D 1998 S11 pg 80*/
          if (forward)
            br_flood_forward (br, skb, cvid, svid, 0);
          else
            kfree_skb(skb);
        }
      BR_READ_UNLOCK ( &br->lock );
      return 0;
    }

#ifdef HAVE_CFM
  if (br_vlan_get_nondot1q_ether_type (skb) == ETH_P_CFM)
    {
      BDEBUG ( "CFM PDU received  %d\n", port->state[instance] );

      br_pass_frame_up_cfm ( br, skb );
      read_unlock ( &br->lock );
      kfree_skb (skb);
      return 0;
    }
#endif /* HAVE_CFM */

  /* Handle the BPDUs by comparing first 5 octets and pass the frame to stp */
  /* If the frame is destinde to the ingress port or to any of the local port */
  if ( (!(memcmp ( dest_addr, group_addr, 6 )) &&
          (br->type == BR_TYPE_STP ||
           br->type == BR_TYPE_RSTP ||
           br->type == BR_TYPE_MSTP ||
           br->num_ce_ports)) ||
           ((!memcmp(dest_addr,pbb_oui,3))&&
           (!memcmp(skb->mac_header+24,pro_group_addr,6 ))
           && br->num_cnp_ports) ||
       (br_vlan_is_dest_mac_addr_local(skb)) )
    {
      /* Currently the other multicast frames are also being passed up
         to br_handle_frame_finish for flooding. Later when we have a
         configurable group address, we should flood the other entries  */
      BDEBUG ( "BPDU received  %d\n", port->state[instance] );

      br_pass_frame_up ( br, skb );
      BR_READ_UNLOCK ( &br->lock );
      kfree_skb (skb);
      return 0;
    }
  /* Handle the RPVST BPDUs and pass the frame up */
  if ( ((!memcmp (dest_addr, rpvst_group_addr, 6)) ||
        (!memcmp (dest_addr, group_addr, 6)) ) &&
       (br->type == BR_TYPE_RPVST ))
    {
      /* Currently the other multicast frames are also being passed up
         to br_handle_frame_finish for flooding. Later when we have a
         configurable group address, we should flood the other entries  */
      BDEBUG( "SSTP BPDU received \n");
      br_pass_frame_up ( br, skb );
      BR_READ_UNLOCK ( &br->lock );
      kfree_skb (skb);
      return 0;
    }

  if (memcmp ( dest_addr, group_addr, 6 ) == 0)
    {
      br_provider_vlan_flood (br, skb, cvid, svid, 0);
      read_unlock ( &br->lock );
      kfree_skb (skb);
      return 0;
    }

  if ( (memcmp ( dest_addr, pro_group_addr, 6 ) == 0) &&
        (br->type == BR_TYPE_PROVIDER_RSTP ||
         br->type == BR_TYPE_PROVIDER_MSTP))
    {
      BDEBUG ( "Provider BPDU received  %d\n", port->state[instance] );

      br_pass_frame_up ( br, skb );
      BR_READ_UNLOCK ( &br->lock );
      kfree_skb (skb);
      return 0;
    }

  BDEBUG ("XXX %x %x %x %x %x %x : %d : %d\n", dest_addr[0], dest_addr[1],
          dest_addr[2], dest_addr[3], dest_addr[4], dest_addr[5],
          skb->nh.iph->protocol, br->flags);

  /* Handle the Multicast address for IGMP/PIM/DVMRP by comparing first 5 octets and pass the frame to igmp */
  if  ( (!memcmp (dest_addr, igmp_group_addr, 3))
        && (skb_ip_protocol(skb) == IPPROTO_IGMP)
        && (br->flags & APNBR_IGMP_SNOOP_ENABLED) )
    {
      /* Currently the other multicast frames are also being passed up
         to br_handle_frame_finish for flooding. Later when we have a
         configurable group address, we should flood the other entries  */
      BDEBUG ( "IGMP Multicast PDU received  %d\n", port->state[instance] );

      br_pass_frame_up_igmp_snoop ( br, skb );
      BR_READ_UNLOCK ( &br->lock );
      kfree_skb (skb);
      return 0;
    }

  /* Do not forward reserved mac addresses */
  if ((memcmp (dest_addr, reserved_addr_start, 6) >= 0)
      && (memcmp (dest_addr, reserved_addr_end, 6) <= 0)
      && (br->type != BR_TYPE_PROVIDER_RSTP)
      && (br->type != BR_TYPE_PROVIDER_MSTP))
    {
      BDEBUG ( " Reserved mac: Do not forward \n");
      BR_READ_UNLOCK ( &br->lock );
      kfree_skb (skb);
      return 0;
    }

  BDEBUG ( "Is state fwding? state %d forwarding %d\n",
           port->state[instance], forward );

  /* Pass the frame to the Forwarding Process */
  if ( forward )
    {
      if (port->port_type == CUST_EDGE_PORT)
        {
          skb = br_handle_provider_edge_config (skb, port, cvid, svid);

          /* In order for this CEP to forward any data frame, that frame should 
           * have a s-tag at least */
          u_int8_t frame_type;
          frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_STAG);

          if (frame_type == UNTAGGED)
            {
              skb = br_vlan_push_tag_header (skb, svid, ETH_P_8021Q_STAG);
              skb->mac_header = skb->data;
              skb_pull (skb, ETH_HLEN);
            }
        }

      br_handle_frame_finish ( skb, cvid, svid );
    }
  else
    {
      BDEBUG ( "Port state is not fwding: discarding frame\n" );
      kfree_skb ( skb );
    }

  BR_READ_UNLOCK ( &br->lock );
  return 0;
}
