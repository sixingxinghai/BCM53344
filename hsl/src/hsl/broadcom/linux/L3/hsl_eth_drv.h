/* Copyright (C) 2004  All Rights Reserved. */

#ifndef _HSL_ETH_DRV_H
#define _HSL_ETH_DRV_H


int hsl_eth_drv_init (void);
int hsl_eth_drv_deinit (void);
int hsl_eth_drv_post_l3_pkt (struct hsl_if *ifpl3, bcm_pkt_t *pkt);
int hsl_eth_dev_xmit (struct sk_buff * skb, struct net_device * dev);
int hsl_eth_dev_open (struct net_device * dev);
int hsl_eth_dev_close (struct net_device * dev);
struct net_device_stats *hsl_eth_dev_get_stats(struct net_device *dev);
void hsl_eth_dev_set_mc_list (struct net_device *dev);
int hsl_eth_dev_set_mac_addr (struct net_device *dev, void *p);
struct net_device *hsl_eth_drv_create_netdevice (struct hsl_if *ifp,  u_char *hwaddr, int hwaddrlen);
int hsl_eth_drv_destroy_netdevice (struct net_device *dev);

#endif /* _HSL_ETH_DRV_H */
