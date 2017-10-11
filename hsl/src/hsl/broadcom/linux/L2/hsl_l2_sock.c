/* Copyright (C) 2004   All Rights Reserved. */

#include "config.h"
#include "hsl_os.h"
                                                                                
/* Broadcom includes. */
#include "bcm_incl.h"

#include "hsl_types.h"
                                                                               
/* HAL includes. */
#include "hal_netlink.h"
#include "hal_msg.h"

#include "hsl_avl.h"
#include "hsl.h"
#include "hsl_oss.h"
#include "hsl_comm.h"
#include "hsl_logger.h"
#include "hsl_ifmgr.h"
#include "hsl_ether.h"

#include "hsl_vlan.h"

#include "hsl_bcm_ifmap.h"
#include "hsl_l2_sock.h"

#ifdef HAVE_LACPD
extern int hsl_af_lacp_sock_init ();
extern int hsl_af_lacp_sock_deinit ();
#endif /* HAVE_LACPD */
#if defined(HAVE_STPD) || defined(HAVE_RSTPD) || defined(HAVE_MSTPD)
extern int hsl_af_stp_sock_init ();
extern int hsl_af_stp_sock_deinit ();
#endif /* defined(HAVE_STPD) || defined(HAVE_RSTPD) || defined(HAVE_MSTPD) */
#ifdef HAVE_AUTHD
extern int hsl_af_eapol_sock_init ();
extern int hsl_af_eapol_sock_deinit ();
#endif /* HAVE_AUTHD */
#ifdef HAVE_ONMD
extern int hsl_af_cfm_sock_init ();
extern int hsl_af_cfm_sock_deinit ();
extern int hsl_af_efm_sock_init ();
extern int hsl_af_efm_sock_deinit ();
extern int hsl_af_lldp_sock_init ();
extern int hsl_af_lldp_sock_deinit ();
#endif /* HAVE_ONMD */
#if defined(HAVE_GVRP) || defined(HAVE_GMRP)
extern int hsl_af_garp_sock_init ();
extern int hsl_af_garp_sock_deinit ();
#endif /* defined(HAVE_GVRP) || defined (HAVE_GMRP) */
#ifdef HAVE_IGMP_SNOOP
extern int hsl_af_igs_sock_init ();
extern int hsl_af_igs_sock_deinit ();
#endif /* HAVE_IGMP_SNOOP. */
#ifdef HAVE_MLD_SNOOP
extern int hsl_af_mlds_sock_init ();
extern int hsl_af_mlds_sock_deinit ();
#endif /* HAVE_MLD_SNOOP. */


/* Post a packet to the socket backend. */
int
hsl_sock_post_skb (struct sock *sk, struct sk_buff *skb)
{
  skb_set_owner_r (skb, sk);
  skb->dev = NULL;
#ifdef LINUX_KERNEL_2_6
  spin_lock (&sk->sk_receive_queue.lock);
  __skb_queue_tail (&sk->sk_receive_queue, skb);
  spin_unlock (&sk->sk_receive_queue.lock);
  sk->sk_data_ready (sk, skb->len);
#else
  spin_lock (&sk->receive_queue.lock);
  __skb_queue_tail (&sk->receive_queue, skb);
  spin_unlock (&sk->receive_queue.lock);
  sk->data_ready (sk, skb->len);  
#endif
  return 0;
}

/* Returns a skb with untagged control frame. */
static struct sk_buff *
_hsl_bcm_rx_handle_untagged (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sockaddr_l2 sockaddr;
  int sockaddrlen = sizeof (struct sockaddr_l2);
  int totlen;
  struct sk_buff *skb = NULL;
  u_char *p = NULL;
  struct hsl_eth_header *eth = NULL;
  int payloadlen;
  int len;

  p = BCM_PKT_DMAC (pkt);
  eth = (struct hsl_eth_header *) p;

  if (eth->d.type == 0x8100)
    {
      len = ENET_TAGGED_HDR_LEN;
    }
  else
    {
      len = ENET_UNTAGGED_HDR_LEN;
    }

  payloadlen = pkt->pkt_len - len - 4;

  /* Packet length - len - 4(for CRC) + sizeof (struct sockaddr_l2) */
  totlen = sockaddrlen + payloadlen;

  skb = dev_alloc_skb (totlen);
  if (! skb)
    return NULL;

  /* Set length. */
  skb->len = totlen;
  skb->truesize = totlen;

  /* Fill sockaddr. */
  memcpy (sockaddr.dest_mac, eth->dmac, 6);
  memcpy (sockaddr.src_mac, eth->smac, 6);
  sockaddr.port = ifp->ifindex;

  /* Copy sockaddr. */
  memcpy (skb->data, &sockaddr, sockaddrlen);
  p += len;

  /* Copy packet. */
  memcpy (skb->data + sockaddrlen, p, payloadlen);

  return skb;
}

/* Returns a skb with tagged control frame. */
static struct sk_buff *
_hsl_bcm_rx_handle_tagged (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sockaddr_vlan sockaddr;
  int sockaddrlen = sizeof (struct sockaddr_vlan);
  int totlen;
  struct sk_buff *skb;
  u_char *p;
  struct hsl_eth_header *eth;
  int payloadlen;

  p = BCM_PKT_DMAC (pkt);
  eth = (struct hsl_eth_header *) p;

  /* Actual payload length. */
  payloadlen = pkt->pkt_len - ENET_TAGGED_HDR_LEN - 4;

  /* Packet length - ENET_TAGGED_HDR_LEN - 4(for CRC) + sizeof (struct sockaddr_vlan). */
  totlen = sockaddrlen + payloadlen;

  skb = dev_alloc_skb (totlen);
  if (! skb)
    return NULL;

  /* Set length. */
  skb->len = totlen;
  skb->truesize = totlen;

  /* Fill sockaddr. */
  memcpy (sockaddr.dest_mac, eth->dmac, 6);
  memcpy (sockaddr.src_mac, eth->smac, 6);
  sockaddr.port = ifp->ifindex;
  sockaddr.vlanid = HSL_ETH_VLAN_GET_VID (eth->d.vlan.pri_cif_vid);

  /* Copy sockaddr. */
  memcpy (skb->data, &sockaddr, sockaddrlen);
  p += ENET_TAGGED_HDR_LEN;

  /* Copy packet. */
  memcpy (skb->data + sockaddrlen, p, payloadlen);

  return skb;
}

/* Returns a skb with tagged control frame. */
/* Handle tagged 802.3 packet */
static struct sk_buff *
_hsl_bcm_rx_handle_802_3_tagged (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sockaddr_vlan sockaddr;
  int sockaddrlen = sizeof (struct sockaddr_vlan);
  int totlen;
  struct sk_buff *skb = NULL;
  u_char *p = NULL;
  struct hsl_eth_header *eth = NULL;
  int payloadlen;

  p = BCM_PKT_DMAC (pkt);
  eth = (struct hsl_eth_header *) p;

  /* Actual payload length. */
  payloadlen = ntohs(eth->d.vlan.type);

  /* Packet length - ENET_TAGGED_HDR_LEN - 4(for CRC) + sizeof (struct sockaddr_vlan). */
  totlen = sockaddrlen + payloadlen;

  skb = dev_alloc_skb (totlen);
  if (! skb)
    return NULL;

  /* Set length. */
  skb->len = totlen;
  skb->truesize = totlen;

  /* Fill sockaddr. */
  memcpy (sockaddr.dest_mac, eth->dmac, 6);
  memcpy (sockaddr.src_mac, eth->smac, 6);
  sockaddr.port = ifp->ifindex;
  sockaddr.vlanid = HSL_ETH_VLAN_GET_VID (eth->d.vlan.pri_cif_vid);

  /* Copy sockaddr. */
  memcpy (skb->data, &sockaddr, sockaddrlen);
  p += ENET_TAGGED_HDR_LEN;

  /* Copy packet. */
  memcpy (skb->data + sockaddrlen, p, payloadlen);

  return skb;
}




#if defined (HAVE_STPD) || defined(HAVE_RSTPD) || defined(HAVE_MSTPD)
/*
  Handle BPDU. 
  Substract CRC length.
  pkt -> mblk copy
*/
int
hsl_bcm_rx_handle_bpdu (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sk_buff *skb;

  skb = _hsl_bcm_rx_handle_802_3_tagged (ifp, pkt);
  if (! skb)
    return -1;

  /* Post this skb to the socket backend. */
  hsl_af_stp_post_packet (skb);

  /* Free skb. */
  kfree_skb (skb);

  return 0;
}
#endif /* defined (HAVE_STPD) || defined(HAVE_RSTPD) || defined(HAVE_MSTPD) */

#ifdef HAVE_AUTHD

/* Returns a skb with eapol control frame. */
static struct sk_buff *
_hsl_bcm_rx_handle_eapol (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sockaddr_l2 sockaddr;
  int sockaddrlen = sizeof (struct sockaddr_l2);
  int totlen;
  struct sk_buff *skb = NULL;
  u_char *p = NULL;
  u_int32_t recv_untagged;
  struct hsl_eth_header *eth = NULL;
  int payloadlen;
  int hw_pktlen;
  int len;
  hsl_vid_t pvid;

  p = BCM_PKT_DMAC (pkt);
  eth = (struct hsl_eth_header *) p;
  recv_untagged = pkt->rx_untagged;
  
  if (eth->d.type == 0x8100)
    {
      if (!(recv_untagged)) 
        {
          pvid = hsl_get_pvid (ifp);

          if (pvid == 0)
            return NULL;
          
          if (pvid != HSL_ETH_VLAN_GET_VID (eth->d.vlan.pri_cif_vid))
            {
              /* Discarding tagged EAPOL frame. ANVL TC 3.2 */
              HSL_LOG (HSL_LOG_PKTDRV, HSL_LEVEL_ERROR," Discarding tagged frame \n");
              return NULL;
            }
        }
      /* Priority-tagged frame vlan id is NULL */
      len = ENET_TAGGED_HDR_LEN;
    }
  else
    {
      len = ENET_UNTAGGED_HDR_LEN;
    }
  
  p += len;
  payloadlen = ((p[2] << 8) | p[3]) + HSL_L2_AUTH_HDR_LEN;
  hw_pktlen = BCM_PKT_IEEE_LEN(pkt) - len - 4;

  /* Junk packet */
  if (payloadlen > hw_pktlen)
    return NULL;

  /* Packet length - len - 4(for CRC) + sizeof (struct sockaddr_l2) */
  totlen = sockaddrlen + payloadlen;

  skb = dev_alloc_skb (totlen);
  if (! skb)
    return NULL;

  /* Set length. */
  skb->len = totlen;
  skb->truesize = totlen;

  /* Fill sockaddr. */
  memcpy (sockaddr.dest_mac, eth->dmac, 6);
  memcpy (sockaddr.src_mac, eth->smac, 6);
  sockaddr.port = ifp->ifindex;

  /* Copy sockaddr. */
  memcpy (skb->data, &sockaddr, sockaddrlen);

  /* Copy packet. */
  memcpy (skb->data + sockaddrlen, p, payloadlen);

  return skb;
}

/*
  Handle EAPOL.
  Substract CRC length.
  pkt -> mblk copy
*/
int
hsl_bcm_rx_handle_eapol (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sk_buff *skb;

  skb = _hsl_bcm_rx_handle_eapol (ifp, pkt);
  if (! skb)
    return -1;

  /* Post this skb to the socket backend. */
  hsl_af_eapol_post_packet (skb);

  /* Free skb. */
  kfree_skb (skb);

  return 0;
}

#ifdef HAVE_MAC_AUTH
/*  Handle AUTH-MAC */
static struct sk_buff *
_hsl_bcm_rx_handle_auth_mac (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sockaddr_vlan sockaddr;
  int sockaddrlen = sizeof (struct sockaddr_vlan);
  int totlen;
  struct sk_buff *skb = NULL;;
  u_char *p;
  struct hsl_eth_header *eth;
  int payloadlen;

  p = BCM_PKT_DMAC (pkt);
  eth = (struct hsl_eth_header *) p;

  /* Actual payload length. */
  payloadlen = pkt->pkt_len - ENET_TAGGED_HDR_LEN - 4;

  /* Packet length - ENET_TAGGED_HDR_LEN - 4(for CRC) +
     sizeof (struct sockaddr_stp) */
  totlen = sockaddrlen  + payloadlen;

  skb = dev_alloc_skb (totlen);
  if (skb == NULL)
    return NULL;

  if (BCM_PKT_TAG_PROTOCOL(pkt)==HSL_L2_ETH_P_8021Q)
    {
      /* Fill sockaddr. */
      memcpy (sockaddr.dest_mac, eth->dmac, 6);
      memcpy (sockaddr.src_mac, eth->smac, 6);
      sockaddr.port = ifp->ifindex;
      sockaddr.vlanid = HSL_ETH_VLAN_GET_VID (eth->d.vlan.pri_cif_vid);
    }
  else
    {
      /* Fill sockaddr. */
      memcpy (sockaddr.dest_mac, eth->dmac, 6);
      memcpy (sockaddr.src_mac, eth->smac, 6);
      sockaddr.port = ifp->ifindex;
      sockaddr.vlanid = hsl_get_pvid (ifp);
    }

  /* Copy sockaddr. */
  memcpy (skb->data, &sockaddr, sockaddrlen);
  p += ENET_TAGGED_HDR_LEN;

  /* Copy packet. */
  memcpy (skb->data + sockaddrlen, p, payloadlen);

  return skb;
}

int
hsl_bcm_rx_handle_auth_mac (struct hsl_if *ifpl2, bcm_pkt_t *pkt)
{
  struct sk_buff *skb;

  skb = _hsl_bcm_rx_handle_auth_mac (ifpl2, pkt);
  if (skb == NULL)
    {
      return -1;
    }

  /* Post this mblk to the EAPOL socket backend. */
  hsl_af_eapol_post_packet (skb);

  /* Free the skb. */
  kfree_skb (skb);

  return 0;
}
#endif /* HAVE_MAC_AUTH */
#endif /* HAVE_AUTHD */

#ifdef HAVE_LACPD
/*
  Handle LACP.
  Substract CRC length.
  pkt -> mblk copy
*/
int
hsl_bcm_rx_handle_lacp (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sk_buff *skb;

  skb = _hsl_bcm_rx_handle_untagged (ifp, pkt);
  if (! skb)
    return -1;

  /* Post this skb to the socket backend. */
  hsl_af_lacp_post_packet (skb);

  /* Free skb. */
  kfree_skb (skb);

  return 0;
}
#endif /* HAVE_LACPD */

#ifdef HAVE_GMRP
/*
  Handle GMRP.
  Substract CRC length
  pkt -> mblk copy
*/
int
hsl_bcm_rx_handle_gmrp (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sk_buff *skb;

  skb = _hsl_bcm_rx_handle_tagged (ifp, pkt);
  if (! skb)
    return -1;

  /* Post this skb to the socket backend. */
  hsl_af_garp_post_packet (skb);

  /* Free skb. */
  kfree_skb (skb);

  return 0;
}
#endif /* HAVE_GMRP */

#ifdef HAVE_GVRP
/*
  Handle GVRP.
  Substract CRC length
  pkt -> mblk copy
*/
int
hsl_bcm_rx_handle_gvrp (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sk_buff *skb;

  skb = _hsl_bcm_rx_handle_tagged (ifp, pkt);
  if (! skb)
    return -1;

  /* Post this skb to the socket backend. */
  hsl_af_garp_post_packet (skb);

  /* Free skb. */
  kfree_skb (skb);

  return 0;
}

#endif /* HAVE_GVRP */

#ifdef HAVE_IGMP_SNOOP
/*
  Handle IGMP Snooping packet.
  Substract CRC length.
  pkt -> mblk copy
*/
int
hsl_bcm_rx_handle_igs (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sk_buff *skb;

  skb = _hsl_bcm_rx_handle_tagged (ifp, pkt);
  if (! skb)
    return -1;

  /* Post this skb to the socket backend. */
  hsl_af_igs_post_packet (skb);

  /* Free skb. */
  kfree_skb (skb);

  return 0;
}
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
/*
  Handle MLD Snooping packet.
  Substract CRC length.
  pkt -> mblk copy
*/
int
hsl_bcm_rx_handle_mlds (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sk_buff *skb;

  skb = _hsl_bcm_rx_handle_tagged (ifp, pkt);
  if (! skb)
    return -1;

  /* Post this skb to the socket backend. */
  hsl_af_mlds_post_packet (skb);

  /* Free skb. */
  kfree_skb (skb);

  return 0;
}
#endif /* HAVE_MLD_SNOOP */

#ifdef HAVE_ONMD

int
hsl_bcm_rx_handle_lldp (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sk_buff *skb;

  skb = _hsl_bcm_rx_handle_tagged (ifp, pkt);
  if (! skb)
    return -1;

  /* Post this skb to the socket backend. */
  hsl_af_lldp_post_packet (skb);

  /* Free skb. */
  kfree_skb (skb);

  return 0;
}

int
hsl_bcm_rx_handle_efm (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sk_buff *skb;

  skb = _hsl_bcm_rx_handle_tagged (ifp, pkt);
  if (! skb)
    return -1;

  /* Post this skb to the socket backend. */
  hsl_af_efm_post_packet (skb);

  /* Free skb. */
  kfree_skb (skb);

  return 0;
}

int
hsl_bcm_rx_handle_cfm (struct hsl_if *ifp, bcm_pkt_t *pkt)
{
  struct sk_buff *skb;

  skb = _hsl_bcm_rx_handle_tagged (ifp, pkt);
  if (! skb)
    return -1;

  /* Post this skb to the socket backend. */
  hsl_af_cfm_post_packet (skb);

  /* Free skb. */
  kfree_skb (skb);

  return 0;
}

#endif /* HAVE_ONMD */

/*
  Initialize L2 socket backends.
*/
int
hsl_l2_sock_init ()
{
#ifdef HAVE_LACPD
  /* LACP. */
  hsl_af_lacp_sock_init ();
#endif /* HAVE_LACPD. */

#if defined (HAVE_STPD) || defined (HAVE_RSTPD) || defined (HAVE_MSTPD)
  /* STP. */
  hsl_af_stp_sock_init ();
#endif /* defined (HAVE_STPD) || defined (HAVE_RSTPD) || defined (HAVE_MSTPD) */

#ifdef HAVE_AUTHD
  /* EAPOL. */
  hsl_af_eapol_sock_init ();
#endif /* HAVE_AUTHD */

#if defined(HAVE_GVRP) || defined(HAVE_GMRP)
  /* GARP. */
  hsl_af_garp_sock_init ();
#endif /* HAVE_GMRP */

#ifdef HAVE_IGMP_SNOOP
  /* IGS. */
  hsl_af_igs_sock_init ();
#endif /* HAVE_IGMP_SNOOP */

#ifdef HAVE_MLD_SNOOP
  /* MLDS. */
  hsl_af_mlds_sock_init ();
#endif /* HAVE_MLD_SNOOP */

#ifdef HAVE_ONMD
  hsl_af_cfm_sock_init ();
  hsl_af_efm_sock_init ();
  hsl_af_lldp_sock_init ();
#endif /* HAVE_ONMD */

  return 0;
}

/*
  Deinitialize L2 socket backends.
*/
int
hsl_l2_sock_deinit ()
{
#ifdef HAVE_LACPD
  /* LACP. */
  hsl_af_lacp_sock_deinit ();
#endif /* HAVE_LACD */

#if defined (HAVE_STPD) || defined(HAVE_RSTPD) || defined(HAVE_MSTPD)
  /* STP. */
  hsl_af_stp_sock_deinit ();
#endif /* defined (HAVE_STPD) || defined(HAVE_RSTPD) || defined(HAVE_MSTPD) */

#ifdef HAVE_AUTHD
  /* EAPOL. */
  hsl_af_eapol_sock_deinit ();
#endif /* HAVE_AUTHD */

#if defined(HAVE_GMRP) || defined(HAVE_GVRP)
  /* GARP. */
  hsl_af_garp_sock_deinit ();
#endif /* defined(HAVE_GMRP) || defined(HAVE_GVRP) */

#ifdef HAVE_IGMP_SNOOP
  /* IGS. */
  hsl_af_igs_sock_deinit ();
#endif /* HAVE_IGMP_SNOOP. */

#ifdef HAVE_MLD_SNOOP
  /* MLDS. */
  hsl_af_mlds_sock_deinit ();
#endif /* HAVE_MLD_SNOOP. */

#ifdef HAVE_ONMD
  hsl_af_cfm_sock_deinit ();
  hsl_af_efm_sock_deinit ();
  hsl_af_lldp_sock_deinit ();
#endif /* HAVE_ONMD */

  return 0;
}
