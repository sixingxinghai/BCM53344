
#define CFM_GET_LEVEL_DEST_ADDR_MASK           0x0f
#define CFM_DEFAULT_LINK_LEVEL                 0

int
cfm_rcv (struct sk_buff * skb, struct apn_bridge * bridge,
           struct net_device * port);

