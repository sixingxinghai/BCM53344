#include <linux/autoconf.h>
#include "br_types.h"
#include "br_avl.h"
#include "bdebug.h"
#include "br_vlan.h"
#include "br_vlan_api.h"

int
br_vlan_trans_tab_ent_cmp (void *data1, void *data2)
{

  struct br_vlan_trans_key *key1 = data1;
  struct br_vlan_trans_key *key2 = data2;

  if ((!data1) || (!data2))
    return -1;

  if (key1->vid > key2->vid)
    return 1;

  if (key2->vid > key1->vid)
    return -1;

  return 0;
}

int
br_vlan_rev_trans_tab_ent_cmp (void *data1, void *data2)
{

  struct br_vlan_trans_key *key1 = data1;
  struct br_vlan_trans_key *key2 = data2;

  if ((!data1) || (!data2))
    return -1;

  if (key1->trans_vid > key2->trans_vid)
    return 1;

  if (key2->trans_vid > key1->trans_vid)
    return -1;

  return 0;
}

int
br_vlan_reg_tab_ent_cmp (void *data1, void *data2)
{

  struct br_vlan_regis_key *key1 = data1;
  struct br_vlan_regis_key *key2 = data2;

  if ((!data1) || (!data2))
    return -1;

  if (key1->cvid > key2->cvid)
    return 1;

  if (key2->cvid > key1->cvid)
    return -1;

  return 0;
}

int
br_vlan_pro_edge_port_tab_ent_cmp (void *data1, void *data2)
{

  struct br_vlan_pro_edge_port *key1 = data1;
  struct br_vlan_pro_edge_port *key2 = data2;

  if ((!data1) || (!data2))
    return -1;

  if (key1->svid > key2->svid)
    return 1;

  if (key2->svid > key1->svid)
    return -1;

  return 0;
}

int
br_vlan_trans_tab_entry_add (char *bridge_name, unsigned int port_no,
                             vid_t vid, vid_t trans_vid)
{
  int ret;
  struct br_avl_node *node;
  struct apn_bridge *br = 0;
  struct br_vlan_trans_key key;
  struct apn_bridge_port *port = 0;
  struct br_vlan_trans_key *tmpkey;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  BR_WRITE_LOCK_BH (&br->lock);

  if (! port
      || ! port->trans_tab
      || ! port->rev_trans_tab)
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }

  key.vid = vid;
  key.trans_vid = trans_vid;

  node = br_avl_lookup (port->trans_tab, (u_char *) &key);

  if (node && (tmpkey = BR_AVL_NODE_INFO (node)))
    {
       br_avl_delete (port->trans_tab, tmpkey);
       br_avl_delete (port->rev_trans_tab, tmpkey);
       kfree (tmpkey);
    }

  node = br_avl_lookup (port->rev_trans_tab, (u_char *) &key);

  if (node && (tmpkey = BR_AVL_NODE_INFO (node)))
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }

  tmpkey = kmalloc ( sizeof (struct br_vlan_trans_key), GFP_ATOMIC);

  if (tmpkey == NULL)
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -ENOMEM;
    }

  tmpkey->vid = vid;
  tmpkey->trans_vid = trans_vid;

  ret = br_avl_insert (port->trans_tab, (void *) tmpkey);

  if (ret < 0)
    {
      kfree (tmpkey);
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -ENOMEM;
    }

  ret = br_avl_insert (port->rev_trans_tab, (void *) tmpkey);

  if (ret < 0)
    {
      kfree (tmpkey);
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -ENOMEM;
    }


  BR_WRITE_UNLOCK_BH (&br->lock);
  return 0;

}

int
br_vlan_trans_tab_entry_delete (char *bridge_name, unsigned int port_no,
                                vid_t vid, vid_t trans_vid)
{

  struct br_vlan_trans_key *tmpkey;
  struct apn_bridge_port *port = 0;
  struct br_vlan_trans_key key;
  struct apn_bridge *br = 0;
  struct br_avl_node *node;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  BR_WRITE_LOCK_BH (&br->lock);

  if (! port
      || ! port->trans_tab)
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }

  key.vid = vid;
  key.trans_vid = trans_vid;

  node = br_avl_lookup (port->trans_tab, (u_char *) &key);

  if (node && (tmpkey = BR_AVL_NODE_INFO (node)))
    {
      br_avl_delete (port->trans_tab, tmpkey);
      br_avl_delete (port->rev_trans_tab, tmpkey);
      kfree (tmpkey);
    }

  BR_WRITE_UNLOCK_BH (&br->lock);
  return 0;

}

int
br_vlan_trans_tab_delete (struct apn_bridge_port *port)
{
  struct br_avl_node *node;
  struct br_avl_node *next_node;
  struct br_vlan_trans_key *tmpkey;

  if (! port
      || ! port->trans_tab)
    {
      return 0;
    }

  for (node = br_avl_top (port->trans_tab); node; node = next_node)
     {
       next_node = br_avl_next (node);

       if (! (tmpkey = BR_AVL_NODE_INFO (node)))
         continue;

       kfree (tmpkey);
     }

  br_avl_tree_free (&port->trans_tab, NULL);
  br_avl_tree_free (&port->rev_trans_tab, NULL);
  port->trans_tab = NULL;

  return 0;

}

struct br_vlan_trans_key *
br_vlan_trans_tab_lookup (struct apn_bridge_port *port,
                          vid_t vid)
{
  struct br_vlan_trans_key *tmpkey;
  struct br_vlan_trans_key key;
  struct br_avl_node *node;

  if (! port
      || ! port->trans_tab)
    return NULL;

  key.vid = vid;

  node = br_avl_lookup (port->trans_tab, (u_char *) &key);

  if (node && (tmpkey = BR_AVL_NODE_INFO (node)))
    return tmpkey;

  return NULL;

}

struct br_vlan_trans_key *
br_vlan_rev_trans_tab_lookup (struct apn_bridge_port *port,
                              vid_t trans_vid)
{
  struct br_vlan_trans_key *tmpkey;
  struct br_vlan_trans_key key;
  struct br_avl_node *node;

  if (! port
      || ! port->trans_tab)
    return NULL;

  key.trans_vid = trans_vid;

  node = br_avl_lookup (port->rev_trans_tab, (u_char *) &key);

  if (node && (tmpkey = BR_AVL_NODE_INFO (node)))
    return tmpkey;

  return NULL;

}

int
br_vlan_reg_tab_entry_add (char *bridge_name, unsigned int port_no,
                           vid_t cvid, vid_t svid)
{
  int ret;
  char *config_ports;
  struct br_avl_node *node;
  struct apn_bridge *br = 0;
  struct br_vlan_regis_key key;
  struct apn_bridge_port *port = 0;
  struct br_vlan_regis_key *tmpkey;

  BDEBUG("Entering br_vlan_trans_reg_entry_add \n");

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  BR_WRITE_LOCK_BH (&br->lock);

  if (! port
      || ! port->reg_tab)
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }

  key.cvid = cvid;
  key.svid = svid;

  if (port->port_type != CUST_EDGE_PORT)
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }

  node = br_avl_lookup (port->reg_tab, (u_char *) &key);

  if (node && (tmpkey = BR_AVL_NODE_INFO (node)))
    {
      if (tmpkey->svid == svid)
        {
          BR_WRITE_UNLOCK_BH (&br->lock);
          return 0;
        }

       br_avl_delete (port->reg_tab, tmpkey);
       kfree (tmpkey);
    }

  tmpkey = kmalloc ( sizeof (struct br_vlan_regis_key), GFP_ATOMIC);

  if (tmpkey == NULL)
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -ENOMEM;
    }

  tmpkey->cvid = cvid;
  tmpkey->svid = svid;
  tmpkey->state_flags = 0xff;

  ret = br_avl_insert (port->reg_tab, (void *) tmpkey);

  if (ret < 0)
    {
      kfree (tmpkey);
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -ENOMEM;
    }

  config_ports = br->static_svlan_reg_table[svid];

  if (!config_ports)
    {
      config_ports = kmalloc (BR_MAX_PORTS * sizeof (char), GFP_ATOMIC);

      if (!config_ports)
        {
          br_avl_delete (port->reg_tab, tmpkey);
          kfree (tmpkey);
          BDEBUG ("Memory allocation failure for config_ports\n");
          /* RELEASE WRITE LOCK */
          BR_WRITE_UNLOCK_BH (&br->lock);
          return -ENOMEM;
        }

      memset (config_ports, 0, BR_MAX_PORTS * sizeof (char));
      br->static_svlan_reg_table[svid] = config_ports;
      config_ports[port->port_id] |= VLAN_PORT_CONFIGURED;
      config_ports[port->port_id] |= VLAN_REGISTRATION_FIXED;
    }

  BR_WRITE_UNLOCK_BH (&br->lock);
  return 0;

}

int
br_vlan_reg_tab_entry_delete (char *bridge_name, unsigned int port_no,
                              vid_t cvid, vid_t svid)
{

  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;
  struct br_vlan_regis_key *tmpkey;
  struct br_vlan_regis_key key;
  struct br_avl_node *node;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  BDEBUG("Entering br_vlan_trans_tab_entry_delete\n");

  BR_WRITE_LOCK_BH (&br->lock);

  if (! port
      || ! port->reg_tab)
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }

  key.cvid = cvid;
  key.svid = svid;

  node = br_avl_lookup (port->reg_tab, (u_char *) &key);

  if (node && (tmpkey = BR_AVL_NODE_INFO (node)))
    {
       br_avl_delete (port->reg_tab, tmpkey);
       kfree (tmpkey);
    }

  BR_WRITE_UNLOCK_BH (&br->lock);

  return 0;

}

int
br_vlan_reg_tab_delete (struct apn_bridge_port *port)
{
  struct br_avl_node *node;
  struct br_avl_node *next_node;
  struct br_vlan_regis_key *tmpkey;

  if (! port
      || ! port->reg_tab)
    {
      return 0;
    }

  for (node = br_avl_top (port->reg_tab); node; node = next_node)
     {
       next_node = br_avl_next (node);

       if (! (tmpkey = BR_AVL_NODE_INFO (node)))
         continue;

       kfree (tmpkey);
     }

  br_avl_tree_free (&port->reg_tab, NULL);

  return 0;
}


vid_t
br_vlan_svid_get (struct apn_bridge_port *port, vid_t cvid)
{
  struct br_vlan_regis_key *tmpkey;
  struct br_vlan_regis_key key;
  struct br_avl_node *node;

  if (! port
      || ! port->reg_tab)
    {
      return -EINVAL;
    }

  key.cvid = cvid;

  node = br_avl_lookup (port->reg_tab, (u_char *) &key);

  if (node && (tmpkey = BR_AVL_NODE_INFO (node)))
    {
      return tmpkey->svid;
    }

  return VLAN_NULL_VID;

}

unsigned char
br_cvlan_state_flags_get (struct apn_bridge_port *port, vid_t cvid)
{
  struct br_vlan_regis_key *tmpkey;
  struct br_vlan_regis_key key;
  struct br_avl_node *node;

  if (! port
      || ! port->reg_tab)
    {
      return -EINVAL;
    }

  key.cvid = cvid;

  node = br_avl_lookup (port->reg_tab, (u_char *) &key);

  if (node && (tmpkey = BR_AVL_NODE_INFO (node)))
    {
      return tmpkey->state_flags;
    }

  return 0;

}

int
br_vlan_add_pro_edge_port (char *bridge_name, unsigned int port_no,
                           vid_t svid)
{
  int ret;
  struct br_avl_node *node;
  struct apn_bridge *br = 0;
  struct br_vlan_pro_edge_port key;
  struct apn_bridge_port *port = 0;
  struct br_vlan_pro_edge_port *tmpkey;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  BR_WRITE_LOCK_BH (&br->lock);

  if (! port
      || ! port->pro_edge_tab)
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }

  key.svid = svid;

  node = br_avl_lookup (port->pro_edge_tab, (u_char *) &key);

  if (node && (tmpkey = BR_AVL_NODE_INFO (node)))
    {
       BR_WRITE_UNLOCK_BH (&br->lock);
       return 0;
    }

  tmpkey = kmalloc ( sizeof (struct br_vlan_pro_edge_port), GFP_ATOMIC);

  if (tmpkey == NULL)
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -ENOMEM;
    }

  tmpkey->svid = svid;
  tmpkey->pvid = VLAN_NULL_VID;
  tmpkey->untagged_vid = VLAN_NULL_VID;

  ret = br_avl_insert (port->pro_edge_tab, (void *) tmpkey);

  if (ret < 0)
    {
      kfree (tmpkey);
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -ENOMEM;
    }

  BR_WRITE_UNLOCK_BH (&br->lock);
  return 0;

}

int
br_vlan_del_pro_edge_port (char *bridge_name, unsigned int port_no,
                           vid_t svid)
{

  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;
  struct br_vlan_pro_edge_port *tmpkey;
  struct br_vlan_pro_edge_port key;
  struct br_avl_node *node;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  BR_WRITE_LOCK_BH (&br->lock);

  if (! port
      || ! port->pro_edge_tab)
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }

  key.svid = svid;

  node = br_avl_lookup (port->pro_edge_tab, (u_char *) &key);

  if (node && (tmpkey = BR_AVL_NODE_INFO (node)))
    {
       br_avl_delete (port->pro_edge_tab, tmpkey);
       kfree (tmpkey);
    }

  BR_WRITE_UNLOCK_BH (&br->lock);
  return 0;

}

struct br_vlan_pro_edge_port *
br_vlan_pro_edge_port_lookup (struct apn_bridge_port *port, vid_t svid)
{
  struct br_vlan_pro_edge_port *tmpkey;
  struct br_vlan_pro_edge_port key;
  struct br_avl_node *node;

  if (! port
      || ! port->pro_edge_tab)
    {
      return NULL;
    }

  key.svid = svid;

  node = br_avl_lookup (port->pro_edge_tab, (u_char *) &key);

  if (node && (tmpkey = BR_AVL_NODE_INFO (node)))
    {
      return tmpkey;
    }

  return NULL;

}

int
br_vlan_pro_edge_tab_delete (struct apn_bridge_port *port)
{
  struct br_avl_node *node;
  struct br_avl_node *next_node;
  struct br_vlan_pro_edge_port *tmpkey;

  if (! port
      || ! port->pro_edge_tab)
    {
      return 0;
    }

  for (node = br_avl_top (port->pro_edge_tab); node; node = next_node)
     {
       next_node = br_avl_next (node);

       if (! (tmpkey = BR_AVL_NODE_INFO (node)))
         continue;

       kfree (tmpkey);
     }

  br_avl_tree_free (&port->reg_tab, NULL);

  return 0;
}

int
br_vlan_set_pro_edge_pvid (char *bridge_name, unsigned int port_no,
                           vid_t svid, vid_t pvid)
{
  int ret;
  struct br_avl_node *node;
  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;
  struct br_vlan_pro_edge_port *pro_edge_port;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  BR_WRITE_LOCK_BH (&br->lock);

  if (! port
      || ! port->pro_edge_tab)
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }


  pro_edge_port = br_vlan_pro_edge_port_lookup (port, svid);

  if (pro_edge_port == NULL)
    {
       BR_WRITE_UNLOCK_BH (&br->lock);
       return -EINVAL;
    }

  pro_edge_port->pvid = pvid;

  BDEBUG("Setting Provider Edge Port %s.%d PVID %d \n", port->dev->name, pvid);
  BR_WRITE_UNLOCK_BH (&br->lock);

  return 0;

}

int
br_vlan_set_pro_edge_untagged_vid (char *bridge_name, unsigned int port_no,
                                   vid_t svid, vid_t untagged_vid)
{
  int ret;
  struct br_avl_node *node;
  struct apn_bridge *br = 0;
  struct apn_bridge_port *port = 0;
  struct br_vlan_pro_edge_port *pro_edge_port;

  BR_VLAN_GET_VLAN_AWARE_BRIDGE();

  BR_VLAN_GET_PORT();

  BR_WRITE_LOCK_BH (&br->lock);

  if (! port
      || ! port->pro_edge_tab)
    {
      BR_WRITE_UNLOCK_BH (&br->lock);
      return -EINVAL;
    }


  pro_edge_port = br_vlan_pro_edge_port_lookup (port, svid);

  if (pro_edge_port == NULL)
    {
       BR_WRITE_UNLOCK_BH (&br->lock);
       return -EINVAL;
    }

  pro_edge_port->untagged_vid = untagged_vid;

  BDEBUG("Setting Provider Edge Port %s.%d Untagged VID %d \n",
         port->dev->name, untagged_vid);

  BR_WRITE_UNLOCK_BH (&br->lock);
  return 0;

}

struct sk_buff *
br_handle_provider_edge_config (struct sk_buff *skb, struct apn_bridge_port *port,
                                vid_t cvid, vid_t svid)
{
  struct br_vlan_pro_edge_port *pro_edge_port = 0;
  br_frame_t frame_type;

  frame_type = br_vlan_get_frame_type (skb, ETH_P_8021Q_CTAG);

  pro_edge_port = br_vlan_pro_edge_port_lookup (port, svid);

  if (pro_edge_port == NULL)
    return skb;

  if ((frame_type == UNTAGGED) && (pro_edge_port->untagged_vid != cvid))
    {
      skb = br_vlan_push_tag_header (skb, cvid, ETH_P_8021Q_CTAG);
      skb->mac_header = skb->data;
      skb_pull (skb, ETH_HLEN);
      return skb;
    }

  if (frame_type == PRIORITY_TAGGED && pro_edge_port->untagged_vid != cvid)
    {
      skb = br_vlan_pop_tag_header (skb);
      skb_pull (skb, ETH_HLEN);
      skb = br_vlan_push_tag_header (skb, cvid, ETH_P_8021Q_CTAG);
      skb->mac_header = skb->data;
      skb_pull (skb, ETH_HLEN);
      return skb;
    }

  if (frame_type == TAGGED && pro_edge_port->untagged_vid == cvid)
    {
      skb = br_vlan_pop_tag_header (skb);
      skb->mac_header = skb->data;
      skb_pull (skb, ETH_HLEN);
      return skb;
    }

  return skb;
}

void
br_vlan_translate_ingress_vid (struct apn_bridge_port *port,
                               struct sk_buff *skb)
{
  struct br_vlan_trans_key *entry;
  br_frame_t frame_type;
  unsigned short proto;
  vid_t vid;

  entry = NULL;

  if (port->port_type == PRO_NET_PORT
      || port->port_type == CUST_NET_PORT)
    {
      proto = ETH_P_8021Q_STAG;
      frame_type = br_vlan_get_frame_type (skb, proto);
    }
  else
    {
      proto = ETH_P_8021Q_CTAG;
      frame_type = br_vlan_get_frame_type (skb, proto);
    }

  if (frame_type == UNTAGGED || frame_type == PRIORITY_TAGGED)
    return;

  vid = br_vlan_get_vid_from_frame (proto, skb);

  entry = br_vlan_trans_tab_lookup (port, vid);

  if (entry == NULL)
    return;

  br_vlan_swap_tag_header (skb, entry->trans_vid);

}

void
br_vlan_translate_egress_vid (struct apn_bridge_port *port,
                              struct sk_buff *skb)
{
  struct br_vlan_trans_key *entry;
  br_frame_t frame_type;
  unsigned short proto;
  vid_t relay_vid = 0 ;

  entry = NULL;

  BDEBUG("Entering br_vlan_translate_egress_vid \n");

  skb_pull (skb, ETH_HLEN);

  if (port->port_type == PRO_NET_PORT
      || port->port_type == CUST_NET_PORT)
    {
      proto = ETH_P_8021Q_STAG;
      frame_type = br_vlan_get_frame_type (skb, proto);

      if (frame_type == UNTAGGED)
        {
          /* In case of double tagged frames , there will already be a ctag */
          proto = ETH_P_8021Q_CTAG;
          frame_type = br_vlan_get_frame_type (skb, proto);
        }
    }
  else
    {
      proto = ETH_P_8021Q_CTAG;
      frame_type = br_vlan_get_frame_type (skb, proto);
    }

  if (frame_type == UNTAGGED || frame_type == PRIORITY_TAGGED)
    {
      BDEBUG("No-op for untagged frame \n");
      skb_push (skb, ETH_HLEN);
      return;
    }

  relay_vid = br_vlan_get_vid_from_frame (proto, skb);

  entry = br_vlan_rev_trans_tab_lookup (port, relay_vid);

  if (entry == NULL)
    {
      BDEBUG("No-op for untagged frame \n");

      if (port->port_type == PRO_NET_PORT)
        {
          /* If there is ctag in the frame, then copy the priority  
           * from c-tag to s-tag */
          if (proto == ETH_P_8021Q_CTAG)
            {
              /* Since, the frame going out on PNP contains a ctag,
               * it is a double tagged frame. So, stag also is present 
               * in the frame */
              short priority = 0;
              short dei = 0;
              unsigned short vlan_tci = 0;
              struct vlan_header *cvlan_hdr = NULL;
              struct vlan_header *svlan_hdr = NULL;

              cvlan_hdr = (struct vlan_header *) 
                          (skb->data + sizeof (struct vlan_header));
    
              svlan_hdr = (struct vlan_header *)skb->data;

      
              priority = ((ntohs(cvlan_hdr->vlan_tci) >> 13) 
                           & VLAN_USER_PRIORITY_MASK );
 
              dei = ((ntohs(cvlan_hdr->vlan_tci) >> 12) 
                           & VLAN_DROP_ELIGIBILITY_MASK);

              vlan_tci |= ((ntohs(svlan_hdr->vlan_tci) & 0x0fff));
              vlan_tci |= (priority << 13); 
              vlan_tci |= (dei <<12);
              
              vlan_tci = htons (vlan_tci);

              svlan_hdr->vlan_tci = vlan_tci;
            }/* if (proto == ETH_P_8021Q_CTAG) */

        }/* if (port->port_type == PRO_NET_PORT) */

      skb_push (skb, ETH_HLEN);
      return;
    }

  br_vlan_swap_tag_header (skb, entry->vid);
  skb_push (skb, ETH_HLEN);
}
